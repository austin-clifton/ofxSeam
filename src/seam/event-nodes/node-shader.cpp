#include "node-shader.h"
#include "../imgui-utils/properties.h"
#include "../shader-utils.h"

using namespace seam;

ShaderNode::ShaderNode() : IEventNode("Shader Material") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// define output pin... this should be doable in the constructor...
	pin_out_material.pin = SetupPinOutput(PinType::MATERIAL, "material");
}

ShaderNode::~ShaderNode() {
	// TODO dealloc pin input names
}

PinInput* ShaderNode::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* ShaderNode::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_material;
}

void ShaderNode::Draw() {
	// TODO set uniforms
}

void ShaderNode::DrawToScreen() {

}

void ShaderNode::GuiDrawNodeView() {

}

bool ShaderNode::GuiDrawPropertiesList() {
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