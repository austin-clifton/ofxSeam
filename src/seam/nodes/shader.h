#pragma once

#include "iNode.h"
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

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

        void OnPinConnected(PinConnectedArgs args) override;

	private:
		bool AttemptShaderLoad(const std::string& shader_name );

		std::vector<char> pinBuffer;

		std::string shader_name = "simple-glass";
		ofShader shader;
		ofFbo fbo;

		glm::ivec2 tex_size = glm::ivec2(1920, 1080);
		
		// input pins are dynamically created based on the shader's uniforms
		std::vector<PinInput> pinInputs;
		PinOutput pinOutMaterial = pins::SetupOutputPin(this, pins::PinType::FBO, "material");
	};
}
