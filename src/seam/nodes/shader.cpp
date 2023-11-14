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
	size = 1;
	return &shaderPin;
}

PinOutput* Shader::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutMaterial;
}

void Shader::OnPinConnected(PinConnectedArgs args) {
	if (args.pinOut->id == pinOutMaterial.id) {
		ofFbo* fbos = { &fbo };
		args.pushPatterns->Push(pinOutMaterial, &fbos, 1);
	}
}

void Shader::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(ofFloatColor(0.0f));
	shader.begin();

	uniformsPin.SetShaderUniforms(&shaderPin, shader);

	fbo.draw(0, 0);

	shader.end();
	fbo.end();
}

bool Shader::AttemptShaderLoad(const std::string& shader_name) {
	if (ShaderUtils::LoadShader(shader, "screen-rect.vert", shader_name + ".frag")) {
		uniformsPin.UpdateUniforms(&shaderPin, shader);
		return true;
	}
	return false;
}

bool Shader::GuiDrawPropertiesList(UpdateParams* params) {
	// first ask the GUI to draw an input for the shader path.
	// if the shader paths changes, then also attempt re-loading the shader.
	return props::DrawShaderPath("shader name", shader_name)
		&& AttemptShaderLoad(shader_name);
}