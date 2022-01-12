#pragma once

#include "i-node.h"
#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Generic custom shader node.
	/// Once its shader name has been set, it will load (or reload) its GLSL files,
	/// and create PinInputs for each uniform variable.
	/// "Outputs" the shader as a Material PinOutput
	class Shader : public INode {
	public:
		Shader();
		~Shader();

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
