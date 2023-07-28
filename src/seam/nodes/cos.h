#pragma once

#include "i-node.h"

#include "../pins/pin-float.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Cosine signal generator
	class Cos : public INode {
	public:
		Cos();
		~Cos();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		float Calculate(float t);

		PinFloatMeta frequencyMeta = PinFloatMeta(0.0001f);

		float frequency = 1.f;
		float amplitude = 0.5f;
		float amplitude_shift = 0.f;
		float phase_shift = 0.f;
		
		std::array<PinInput, 4> pin_inputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &frequency, 1, "Frequency", sizeof(float), &frequencyMeta,
				"a frequency of one will oscillate once per second; two will oscillate twice per second, etc."),
			pins::SetupInputPin(PinType::FLOAT, this, &amplitude, 1, "Amplitude", sizeof(float), nullptr, "all values are multiplied by this number"),
			pins::SetupInputPin(PinType::FLOAT, this, &amplitude_shift, 1, "Amplitude_Shift", sizeof(float), nullptr, "is added to all values"),
			pins::SetupInputPin(PinType::FLOAT, this, &phase_shift, 1, "Phase_Shift", sizeof(float), nullptr, "offsets oscillations"),
		};

		PinOutput pin_out_fval = pins::SetupOutputPin(this, pins::PinType::FLOAT, "output");
	};
}