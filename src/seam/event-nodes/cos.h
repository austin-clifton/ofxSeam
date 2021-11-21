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

		/// number of cycles the cos wave should make in one second
		float frequency = 1.f;
		
		float amplitude = 1.f;

		float amplitude_shift = 0.f;

		float phase_shift = 0.f;

		std::array<PinInput, 4> pin_inputs;

		PinOutput pin_out_fval;
	};
}