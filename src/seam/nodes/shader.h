#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

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

		std::vector<props::NodeProperty> GetProperties() override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		bool AttemptShaderLoad(const std::string& shader_name );

		UniformsPinMap uniformsPin = UniformsPinMap(this, &shader);
		PinInput shaderPin = uniformsPin.SetupUniformsPin("Shader");

		std::string shader_name = "daedelusSmear";
		ofShader shader;
		ofFbo fbo;

		PinOutput pinOutMaterial = pins::SetupOutputStaticFboPin(this, &fbo, pins::PinType::FboRgba16F, "Image");
	};
}
