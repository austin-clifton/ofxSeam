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
    std::vector<PinInput> newPins = UniformsToPinInputs(shader, uniformsPin->node, uniformsBuffer);
    
    // Loop over each new pin, and make sure ids and connections are preserved.
    for (auto& pinIn : newPins) {
        PinInput* match = FindPinInByName(uniformsPin, pinIn.name);
        if (match != nullptr) {
            pinIn.id = match->id;
            pinIn.connection = match->connection;
            match->connection = nullptr;
        }
    }

    childPins = newPins;

    uniformsPin->SetChildren(childPins.data(), childPins.size());
    uniformsPin->node->RecacheInputConnections();
}

void UniformsPin::SetShaderUniforms(PinInput* uniformsPin, ofShader& shader) {
    size_t uniformsSize;
	PinInput* uniforms = uniformsPin->PinInputs(uniformsSize);

	for (size_t i = 0; i < uniformsSize; i++) {
		PinInput& pin = uniforms[i];
		size_t size;
		void* channels = pin.GetChannels(size);
		
		// TODO put this somewhere more accessible,
		// and add more type conversions as you add more to UniformsToPinInputs()
		switch (pin.type) {
		case pins::PinType::FLOAT: {
            switch (size) {
                case 1:
    				shader.setUniform1f(pin.name, *(float*)channels);
                    break;
                case 2:
                    shader.setUniform2f(pin.name, *(glm::vec2*)channels);
                    break;
                case 3:
                    shader.setUniform3f(pin.name, *(glm::vec3*)channels);
                    break;
            }
			break;
		}
		case pins::PinType::INT: {
			if (size == 1) {
				shader.setUniform1i(pin.name, *(int*)channels);
			} else {
				shader.setUniform2i(pin.name, ((int*)(channels))[0], ((int*)(channels))[1]);
			}
			break;
		}
		case pins::PinType::UINT: {
			uint32_t* uchans = (uint32_t*)channels;
			if (size == 1) {
				glUniform1ui(shader.getUniformLocation(pin.name), *uchans);
			} else {
				glUniform2ui(shader.getUniformLocation(pin.name), uchans[0], uchans[1]);
			}
			break;
		}
		}
	}
}
