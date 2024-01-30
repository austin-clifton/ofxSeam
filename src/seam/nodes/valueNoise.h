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

		UniformsPin uniformsPinMap;

		glm::uvec2 resolution = glm::uvec2(1024);

		/*
		// nomenclature from http://libnoise.sourceforge.net/glossary/#perlinnoise
		int octaves = 4;
		float frequency = 4.f;
		float lacunarity = 2.f;
		float seed = 0.0f;
		*/

		std::array<PinInput, 1> pinInputs = {
			uniformsPinMap.SetupUniformsPin(this, "Noise")
			/*
			pins::SetupInputPin(PinType::INT, this, &octaves, 1, "Octaves",
				PinInOptions("number of iterations of noise", &octavesMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &frequency, 1, "Frequency", 
				PinInOptions("initial noise frequency for the first octave", &floatMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &lacunarity, 1, "Lacunarity",
				PinInOptions("each octave's frequency is multiplied by this number", &floatMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &seed, 1, "Seed")
			*/
		};

		PinOutput pinOutFbo = pins::SetupOutputPin(this, pins::PinType::FBO_RGBA, "Output");

		ofFbo fbo;
		ofShader shader;
	};
}

