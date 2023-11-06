#pragma once

#include "iNode.h"


using namespace seam::pins;

namespace seam::nodes {
	class TexgenPerlin : public INode {
	public:
		TexgenPerlin();

		~TexgenPerlin();

		void Draw(DrawParams* params) override;

		// bool GuiDrawPropertiesList() override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		PinIntMeta octavesMeta = PinIntMeta(1, 8);
		PinFloatMeta floatMeta = PinFloatMeta(0.001f);

		// nomenclature from http://libnoise.sourceforge.net/glossary/#perlinnoise
		int octaves = 1;
		float frequency = 7.19f;
		float lacunarity = 2.68f;
		float amplitude = 0.5f;
		float persistence = 0.6f;

		std::array<PinInput, 5> pin_inputs = {
			pins::SetupInputPin(PinType::INT, this, &octaves, 1, "Octaves",
				PinInOptions("number of iterations of noise", &octavesMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &frequency, 1, "Frequency", 
				PinInOptions("initial noise frequency for the first octave", &floatMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &lacunarity, 1, "Lacunarity",
				PinInOptions("each octave's frequency is multiplied by this number", &floatMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &amplitude, 1, "Amplitude",
				PinInOptions("peak value of the first octave", &floatMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &persistence, 1, "Persistence",
				PinInOptions("each octave's amplitude is multiplied by this number", &floatMeta)),
		};

		PinOutput pin_out_tex = pins::SetupOutputPin(this, pins::PinType::FBO, "texture");

		// TODO this should be an editable property, but not a pin (resolution shouldn't be changing)
		glm::ivec2 tex_size = glm::ivec2(512, 512);

		const std::string shader_name = "simplex-noise";

		ofFbo fbo;
		ofShader shader;
	};
}

