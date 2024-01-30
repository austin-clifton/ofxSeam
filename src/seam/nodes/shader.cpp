#include "seam/nodes/shader.h"
#include "seam/imguiUtils/properties.h"
#include "seam/shaderUtils.h"

using namespace seam;
using namespace seam::nodes;

Shader::Shader() : INode("Shader Material") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	windowFbos.push_back(WindowRatioFbo(&fbo));
	gui_display_fbo = &fbo;
}

void Shader::Setup(SetupParams* params) {
	AttemptShaderLoad(shader_name);
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
	// Ask the GUI to draw an input for the shader path.
	if (props::DrawShaderPath("Shader Name", shader_name)) {
		return AttemptShaderLoad(shader_name);
	}
	return false;
}

std::vector<props::NodeProperty> Shader::GetProperties() {
	std::vector<props::NodeProperty> properties;
	
	properties.push_back(props::SetupStringProperty("Frag Shader Name", [this](size_t& size) {
		size = 1;
		return &shader_name;
	}, [this](std::string* newName, size_t size) {
		assert(size == 1);
		shader_name = *newName;
		AttemptShaderLoad(shader_name);
	}));

	return properties;
}