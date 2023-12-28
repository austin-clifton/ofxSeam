#include "seam/pins/uniformsPin.h"
#include "seam/pins/pin.h"
#include "seam/nodes/iNode.h"

using namespace seam::pins;
using namespace seam::nodes;

PinInput UniformsPin::SetupUniformsPin(nodes::INode* node, const std::string_view name) {
    std::string pinName = std::string(name) + " Uniforms";
    return SetupInputPin(PinType::STRUCT, node, nullptr, 0, pinName);
}

void UniformsPin::UpdateUniforms(PinInput* uniformsPin, ofShader& shader) {
	// Move the old uniforms buffer to a temp buffer,
	// so uniform values can be preserved and re-set after re-loading.
	std::vector<char> oldUniformsBuffer = std::move(uniformsBuffer);

    std::vector<PinInput> newPins = UniformsToPinInputs(shader, uniformsPin->node, uniformsBuffer);
    
    // Loop over each new pin, and make sure ids and connections are preserved.
    for (auto& pinIn : newPins) {
        PinInput* match = FindPinInByName(uniformsPin, pinIn.name);
        if (match != nullptr) {
            pinIn.id = match->id;
            pinIn.connection = match->connection;
            match->connection = nullptr;

			// Also copy the previously set shader uniform values.
			size_t pinSize, matchSize;
			char* matchBuff = (char*)match->Buffer(matchSize);
			char* pinBuff = (char*)pinIn.Buffer(pinSize);
			size_t bytesToCopy = std::min(match->BufferSize(), pinIn.BufferSize());
			std::copy(matchBuff, matchBuff + bytesToCopy, pinBuff);
        }
    }

	uniformsPin->SetChildren(std::move(newPins));
    uniformsPin->node->RecacheInputConnections();
}

void UniformsPin::SetShaderUniforms(PinInput* uniformsPin, ofShader& shader) {
    size_t uniformsSize;
	PinInput* uniforms = uniformsPin->PinInputs(uniformsSize);

	int textureIndex = 1;

	for (size_t i = 0; i < uniformsSize; i++) {
		PinInput& pin = uniforms[i];
		size_t size;
		void* buffer = pin.Buffer(size);
		
		// TODO put this somewhere more accessible,
		// and add more type conversions as you add more to UniformsToPinInputs()
		switch (pin.type) {
		case pins::PinType::FLOAT: {
            switch (size) {
                case 1:
    				shader.setUniform1f(pin.name, *(float*)buffer);
                    break;
                case 2:
                    shader.setUniform2f(pin.name, *(glm::vec2*)buffer);
                    break;
                case 3:
                    shader.setUniform3f(pin.name, *(glm::vec3*)buffer);
                    break;
            }
			break;
		}
		case pins::PinType::INT: {
			if (size == 1) {
				shader.setUniform1i(pin.name, *(int*)buffer);
			} else {
				shader.setUniform2i(pin.name, ((int*)(buffer))[0], ((int*)(buffer))[1]);
			}
			break;
		}
		case pins::PinType::UINT: {
			uint32_t* uchans = (uint32_t*)buffer;
			if (size == 1) {
				glUniform1ui(shader.getUniformLocation(pin.name), *uchans);
			} else {
				glUniform2ui(shader.getUniformLocation(pin.name), uchans[0], uchans[1]);
			}
			break;
		}
		case pins::PinType::FBO_RGBA: 
		case pins::PinType::FBO_RGBA16F:
		case pins::PinType::FBO_RED:
		{
			ofFbo** fbos = (ofFbo**)buffer;
			// TODO do sizes > 1 need to be handled?
			assert(size == 1);
			if (fbos[0] != nullptr) {
				shader.setUniformTexture(pin.name, fbos[0]->getTexture(), textureIndex);
				textureIndex += 1;
			}
			break;
		}
		}
	}
}
