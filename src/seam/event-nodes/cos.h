#pragma once

#include "event-node.h"

namespace seam {
	/// Cosine signal generator
	class Cos : public IEventNode {
	public:
		Cos();
		~Cos();

		void Update(float time) override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		float Calculate(float t);

		PinFloat<1> pin_frequency = PinFloat<1>(
			"frequency", 
			"a frequency of one will oscillate once per second; two will oscillate twice per second, etc.",
			{ 1.f },
			0.000001f
		);
		PinFloat<1> pin_amplitude = PinFloat<1>(
			"amplitude",
			"all values are multiplied by this number",
			{ .5f }
		);
		PinFloat<1> pin_amplitude_shift = PinFloat<1>(
			"amplitude_shift", 
			"is added to all values"
		);
		PinFloat<1> pin_phase_shift = PinFloat<1>(
			"phase_shift", 
			"offsets oscillations"
		);
		
		std::array<IPinInput*, 4> pin_inputs = {
			&pin_frequency,
			&pin_amplitude,
			&pin_amplitude_shift,
			&pin_phase_shift,
		};

		PinOutput pin_out_fval;
	};
}