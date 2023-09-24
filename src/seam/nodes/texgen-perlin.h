#pragma once

#include "i-node.h"


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

		/*
		PinInt<1> pin_octaves = PinInt<1>(
			"octaves",
			"number of iterations of noise",
			{ 5 }, 1, 8
			);
		PinFloat<1> pin_frequency = PinFloat<1>(
			"frequency",
			"initial noise frequency for the first octave",
			{ 7.19f }, 0.001f
			);
		PinFloat<1> pin_lacunarity = PinFloat<1>(
			"lacunarity",
			"each octave's frequency is multiplied by this number",
			{ 2.68f },
			0.001f
			);
		PinFloat<1> pin_amplitude = PinFloat<1>(
			"amplitude",
			"peak value of the first octave",
			{ .5f },
			0.001f,
			1.f
			);
		PinFloat<1> pin_persistence = PinFloat<1>(
			"persistence",
			"each octave's amplitude is multiplied by this number",
			{ 0.6f },
			0.001f
			);
			*/

		std::array<PinInput, 5> pin_inputs = {
			pins::SetupInputPin(PinType::INT, this, &octaves, 1, "Octaves",
				sizeof(int32_t), &octavesMeta, "number of iterations of noise"),
			pins::SetupInputPin(PinType::FLOAT, this, &frequency, 1, "Frequency", 
				sizeof(float), &floatMeta, "initial noise frequency for the first octave"),
			pins::SetupInputPin(PinType::FLOAT, this, &lacunarity, 1, "Lacunarity",
				sizeof(float), &floatMeta, "each octave's frequency is multiplied by this number"),
			pins::SetupInputPin(PinType::FLOAT, this, &amplitude, 1, "Amplitude",
				sizeof(float), &floatMeta, "peak value of the first octave"),
			pins::SetupInputPin(PinType::FLOAT, this, &persistence, 1, "Persistence",
				sizeof(float), &floatMeta, "each octave's amplitude is multiplied by this number"),
		};

		PinOutput pin_out_tex = pins::SetupOutputPin(this, pins::PinType::FBO, "texture");

		// TODO this should be an editable property, but not a pin (resolution shouldn't be changing)
		glm::ivec2 tex_size = glm::ivec2(512, 512);

		const std::string shader_name = "simplex-noise";

		ofFbo fbo;
		ofShader shader;
	};
}

