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

		void Draw(DrawParams* params) override;

		bool GuiDrawPropertiesList() override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		bool AttemptShaderLoad(const std::string& shader_name );

		std::string shader_name = "step-grid";
		ofShader shader;
		ofFbo fbo;

		glm::ivec2 tex_size = glm::ivec2(1920, 1080);
		
		// input pins are dynamically created based on the shader's uniforms
		std::vector<PinInput> pin_inputs;
		PinOutput pin_out_material = pins::SetupOutputPin(this, pins::PinType::MATERIAL, "material");
	};
}
