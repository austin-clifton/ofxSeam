#pragma once

#include "event-node.h"

namespace seam {
	/// Generic custom shader node.
	/// Once its shader name has been set, it will load (or reload) its GLSL files,
	/// and create PinInputs for each uniform variable.
	/// "Outputs" the shader as a Material PinOutput
	// TODO I'm having a naming crisis
	class ShaderNode : public IEventNode {
	public:
		ShaderNode();
		~ShaderNode();

		void Draw() override;

		bool GuiDrawPropertiesList() override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		std::string shader_name;
		ofShader shader;
		
		// input pins are dynamically created based on the shader's uniforms
		std::vector<IPinInput*> pin_inputs;
		PinOutput pin_out_material;
	};
}
