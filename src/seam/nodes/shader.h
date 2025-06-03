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
		void OnFilteringChanged();
		void OnTextureWrapChanged();

		bool filterLinear = true;
		int textureWrapHorizontal = GL_CLAMP_TO_EDGE;
		int textureWrapVertical = GL_CLAMP_TO_EDGE;

		UniformsPinMap uniformsPin = UniformsPinMap(this, &shader);
		std::array<PinInput, 2> pinInputs = {
			uniformsPin.SetupUniformsPin("Shader"),
			SetupInputPin(PinType::Bool, this, &filterLinear, 1, "Filter Linear", 
				PinInOptions::WithChangedCallbacks(std::bind(&Shader::OnFilteringChanged, this))
			)
		};

		std::string shader_name = "mixer";
		ofShader shader;
		ofFbo fbo;

		PinOutput pinOutMaterial = pins::SetupOutputStaticFboPin(this, &fbo, pins::PinType::FboRgba16F, "Image");
	};
}
