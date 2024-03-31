#include "seam/pins/uniformsPinMap.h"
#include "seam/pins/pin.h"
#include "seam/include.h"

using namespace seam::pins;
using namespace seam::nodes;

UniformsPinMap::UniformsPinMap(nodes::INode* _node, ofShader* _shader) {
	node = _node;
	shader = _shader;
}

PinInput UniformsPinMap::SetupUniformsPin(const std::string_view name) {
    std::string pinName = std::string(name) + " Uniforms";
	PinInput pinIn = SetupInputPin(PinType::STRUCT, node, nullptr, 0, pinName);
	uniformsPinId = pinIn.id;
    return pinIn;
}

void UniformsPinMap::UpdatePins() {
	PinInput* uniformsPin = node->FindPinInput(uniformsPinId);
	assert(uniformsPin != nullptr);

	// Move the old uniforms buffer to a temp buffer,
	// so uniform values can be preserved and re-set after re-loading.
	std::vector<char> oldUniformsBuffer = std::move(uniformsBuffer);

    std::vector<PinInput> newPins = UniformsToPinInputs(*shader, node, uniformsBuffer);
    
    // Loop over each new pin, and make sure ids and connections are preserved.
    for (auto& pinIn : newPins) {
		if (pins::IsFboPin(pinIn.type)) {
			// Use the pin's value changed callback to call shader.setUniformTexture(),
			// so that doesn't have to be called in SetUniforms()
			pinIn.SetOnValueChanged([this, &pinIn]() {
				size_t size;
				void* buf = pinIn.Buffer(size);
				assert(size == 1);
				ofFbo* fbo = (ofFbo*)buf;

				if (fbo != nullptr) {
					uint32_t texLoc = node->Seam().texLocResolver->Bind(&fbo->getTexture());
					shader->begin();
					shader->setUniformTexture(pinIn.name, fbo->getTexture(), texLoc);
					shader->end();
				}
			});

			pinIn.SetOnValueChanging([this, &pinIn]() {
				size_t size;
				void* buf = pinIn.Buffer(size);
				assert(size == 1);
				ofFbo* fbo = (ofFbo*)buf;

				if (fbo != nullptr) {
					node->Seam().texLocResolver->Release(&fbo->getTexture());
				}
			});
		}

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

			// These are called so FBO texture bindings are handled correctly.
			match->OnValueChanging();
			pinIn.OnValueChanged();
        }
    }

	uniformsPin->SetChildren(std::move(newPins));
    uniformsPin->node->RecacheInputConnections();
}

void UniformsPinMap::SetUniforms() {
	PinInput* uniformsPin = node->FindPinInput(uniformsPinId);
    size_t uniformsSize;
	PinInput* uniforms = uniformsPin->PinInputs(uniformsSize);

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
    				shader->setUniform1f(pin.name, *(float*)buffer);
                    break;
                case 2:
                    shader->setUniform2f(pin.name, *(glm::vec2*)buffer);
                    break;
                case 3:
                    shader->setUniform3f(pin.name, *(glm::vec3*)buffer);
                    break;
            }
			break;
		}
		case pins::PinType::INT: {
			if (size == 1) {
				shader->setUniform1i(pin.name, *(int*)buffer);
			} else {
				shader->setUniform2i(pin.name, ((int*)(buffer))[0], ((int*)(buffer))[1]);
			}
			break;
		}
		case pins::PinType::UINT: {
			uint32_t* uchans = (uint32_t*)buffer;
			if (size == 1) {
				glUniform1ui(shader->getUniformLocation(pin.name), *uchans);
			} else {
				glUniform2ui(shader->getUniformLocation(pin.name), uchans[0], uchans[1]);
			}
			break;
		}
		case pins::PinType::FBO_RGBA: 
		case pins::PinType::FBO_RGBA16F:
		case pins::PinType::FBO_RED:
			// Nothing to do, FBOs are bound on value change instead of per-frame.
			break;
		}
	}
}
