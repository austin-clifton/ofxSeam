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

		PinInt pin_octaves = PinInt("octaves", 1, 1, 8);
		PinFloat pin_frequency = PinFloat("frequency", 8.f, 0.001f);
		PinFloat pin_lacunarity = PinFloat("lacunarity", 2.f, 0.001f);
		PinFloat pin_amplitude = PinFloat("amplitude", .5f, 0.001f, 1.f);
		PinFloat pin_persistence = PinFloat("persistence", 0.5f, 0.001f);

		std::array<PinInput, 5> pin_inputs = {
			SetupPinInput(&pin_octaves, this),
			SetupPinInput(&pin_frequency, this),
			SetupPinInput(&pin_lacunarity, this),
			SetupPinInput(&pin_amplitude, this),
			SetupPinInput(&pin_persistence, this)
		};

		PinOutput pin_out_tex;

		// TODO this should be an editable property, but not a pin (resolution shouldn't be changing)
		glm::ivec2 tex_size = glm::ivec2(512, 512);

		// TODO should also be an editable property, with a hot-reload butan!
		std::string shader_name = "simplex-noise";

		ofFbo fbo;
		ofShader shader;
	};
}

