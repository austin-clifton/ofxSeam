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

		void Setup(SetupParams* params) override;

		void Draw(DrawParams* params) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

        void OnPinConnected(PinConnectedArgs args) override;

	private:
		bool AttemptShaderLoad(const std::string& shader_name );

		UniformsPin uniformsPin;
		PinInput shaderPin = uniformsPin.SetupUniformsPin(this, "Shader");

		std::string shader_name = "simple-glass";
		ofShader shader;
		ofFbo fbo;

		PinOutput pinOutMaterial = pins::SetupOutputPin(this, pins::PinType::FBO, "material");
	};
}
