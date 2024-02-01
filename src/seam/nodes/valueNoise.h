#pragma once

#include "seam/nodes/iNode.h"

using namespace seam::pins;

namespace seam::nodes {
	/// @brief Generates a texture which contains a value noise FBM in x component,
	/// and its normals (derivatives) in .yzw,
	/// using inigo quilez's method described here: http://iquilezles.org/articles/morenoise/
	class ValueNoise : public INode {
	public:
		ValueNoise();

		void Setup(SetupParams* params) override;

		void Draw(DrawParams* params) override;

		PinInput* PinInputs(size_t& size) override;
		PinOutput* PinOutputs(size_t& size) override;

        void OnPinConnected(PinConnectedArgs args) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;
		std::vector<props::NodeProperty> GetProperties() override;

	private:
		void ResizeFbo();
		bool ReloadShader();

		PinIntMeta octavesMeta = PinIntMeta(1, 8);
		PinFloatMeta floatMeta = PinFloatMeta(0.001f);

		UniformsPinMap uniformsPinMap = UniformsPinMap(this);

		glm::uvec2 resolution = glm::uvec2(1024);

		std::array<PinInput, 1> pinInputs = {
			uniformsPinMap.SetupUniformsPin("Noise")
		};

		PinOutput pinOutFbo = pins::SetupOutputPin(this, pins::PinType::FBO_RGBA, "Output");

		ofFbo fbo;
		ofShader shader;
	};
}

