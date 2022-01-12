#include "shader.h"
#include "../imgui-utils/properties.h"
#include "../shader-utils.h"

using namespace seam;
using namespace seam::nodes;

Shader::Shader() : INode("Shader Material") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// define output pin... this should be doable in the constructor...
	pin_out_material.pin = pins::SetupOutputPin(this, pins::PinType::MATERIAL, "material");
}

Shader::~Shader() {
	// TODO dealloc pin input names
}

IPinInput** Shader::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* Shader::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_material;
}

void Shader::Draw() {
	// TODO set uniforms
}

bool Shader::GuiDrawPropertiesList() {
	if (props::DrawShaderPath("shader name", shader_name)
		&& ShaderUtils::LoadShader(shader, shader_name)
	) {
		// reloaded
		
		// TODO proper handling of connected input pins being deleted, pins shifting index, etc. etc.
		// just danger assign it for now so I can see it's doing something
		pin_inputs = UniformsToPinInputs(shader, this);
		return true;
	}
	return false;
}