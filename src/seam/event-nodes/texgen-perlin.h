#pragma once

#include "event-node.h"

namespace seam {

	class TexgenPerlin : public IEventNode {
	public:
		TexgenPerlin();

		~TexgenPerlin();

		// TODO make this a utility function so any texture drawing node can use it
		void GuiDrawNodeView() override;

		void GuiDrawPropertiesList() override;
	
		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:

		// nomenclature from http://libnoise.sourceforge.net/glossary/#perlinnoise

		// number of iterations, anything more than 1 is fractal noise
		int octaves = 4;
		// frequency of the first octave
		float frequency = 8.0;
		// each octave's frequency is multiplied by this number
		float lacunarity = 2.0f;
		// amplitude of the first octave
		float amplitude = 0.5f;
		// each octave's amplitude is multiplied by this number
		float persistence = 0.5f;

		// only re-draw the texture when one of its inputs change
		bool dirty = true;

		std::array<PinInput, 5> pin_inputs;

		PinOutput pin_out_tex;

		// TODO this should be an editable property, but not a pin (resolution shouldn't be changing)
		glm::ivec2 tex_size = glm::ivec2(512, 512);

		// TODO should also be an editable property, with a hot-reload butan!
		std::string shader_name = "simplex-noise";

		ofFbo fbo;
		ofShader shader;
	};
}

