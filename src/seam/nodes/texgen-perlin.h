#pragma once

#include "i-node.h"

#include "../pins/pin-float.h"
#include "../pins/pin-int.h"

using namespace seam::pins;

namespace seam::nodes {
	class TexgenPerlin : public INode {
	public:
		TexgenPerlin();

		~TexgenPerlin();

		void Draw(DrawParams* params) override;

		bool GuiDrawPropertiesList() override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:

		// nomenclature from http://libnoise.sourceforge.net/glossary/#perlinnoise

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

		std::array<IPinInput*, 5> pin_inputs = {
			&pin_octaves,
			&pin_frequency,
			&pin_lacunarity,
			&pin_amplitude,
			&pin_persistence,
		};

		PinOutput pin_out_tex = pins::SetupOutputPin(this, pins::PinType::TEXTURE, "texture");

		// TODO this should be an editable property, but not a pin (resolution shouldn't be changing)
		glm::ivec2 tex_size = glm::ivec2(512, 512);

		std::string shader_name = "simplex-noise";

		ofFbo fbo;
		ofShader shader;
	};
}

