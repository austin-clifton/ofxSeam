#pragma once

#include "event-node.h"

namespace seam {
	/// Cosine signal generator
	class Cos : public IEventNode {
	public:
		Cos();
		~Cos();

		void Update(float time) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		float Calculate(float t);

		PinFloat pin_frequency = PinFloat("frequency", 1.f, 0.000001f);
		PinFloat pin_amplitude = PinFloat("amplitude", .5f);
		PinFloat pin_amplitude_shift = PinFloat("amplitude_shift", 0.f);
		PinFloat pin_phase_shift = PinFloat("phase_shift", 0.f);
		
		std::array<PinInput, 4> pin_inputs = {
			SetupPinInput(&pin_frequency, this),
			SetupPinInput(&pin_amplitude, this),
			SetupPinInput(&pin_amplitude_shift, this),
			SetupPinInput(&pin_phase_shift, this)
		};

		PinOutput pin_out_fval;
	};
}