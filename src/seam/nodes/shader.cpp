#include "shader.h"
#include "../imgui-utils/properties.h"
#include "../shader-utils.h"

using namespace seam;
using namespace seam::nodes;

Shader::Shader() : INode("Shader Material") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	tex_size = glm::ivec2(ofGetWindowWidth(), ofGetWindowHeight());

	fbo.allocate(tex_size.x, tex_size.y);

	AttemptShaderLoad(shader_name);

	gui_display_fbo = &fbo;
}

Shader::~Shader() {
	// TODO dealloc pin input names
}

PinInput* Shader::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

PinOutput* Shader::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutMaterial;
}

void Shader::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(ofFloatColor(0.0f));
	shader.begin();

	for (size_t i = 0; i < pinInputs.size(); i++) {
		PinInput& pin = pinInputs[i];
		size_t size;
		void* channels = pin.GetChannels(size);
		
		// TODO put this somewhere more accessible,
		// and add more type conversions as you add more to UniformsToPinInputs()
		switch (pin.type) {
		case pins::PinType::FLOAT: {
			// ohhh this is annoying, is there any way to avoid it?
			if (size == 1) {
				shader.setUniform1f(pin.name, *(float*)channels);
			} else {
				shader.setUniform2f(pin.name, *(glm::vec2*)channels);
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

	fbo.draw(0, 0);

	shader.end();
	fbo.end();
}

bool Shader::AttemptShaderLoad(const std::string& shader_name) {
	if (ShaderUtils::LoadShader(shader, "screen-rect.vert", shader_name + ".frag")) {
		std::vector<seam::pins::PinInput> newPins = UniformsToPinInputs(shader, this, pinBuffer);

		// Loop over each new pin, and make sure ids and connections are preserved.
		for (auto& pinIn : newPins) {
			PinInput* match = FindPinInByName(this, pinIn.name);
			if (match != nullptr) {
				pinIn.id = match->id;
				pinIn.connection = match->connection;
				match->connection = nullptr;
			}
		}

		pinInputs = newPins;
		RecacheInputConnections();

		return true;
	}
	return false;
}

bool Shader::GuiDrawPropertiesList() {
	// first ask the GUI to draw an input for the shader path.
	// if the shader paths changes, then also attempt re-loading the shader.
	return props::DrawShaderPath("shader name", shader_name)
		&& AttemptShaderLoad(shader_name);
}