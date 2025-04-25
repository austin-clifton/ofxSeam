#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

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
			pins::SetupInputPin(PinType::Float, this, &frequency, 1, "Frequency",
				PinInOptions("a frequency of one will oscillate once per second; two will oscillate twice per second, etc.", &frequencyMeta)),
			pins::SetupInputPin(PinType::Float, this, &amplitude, 1, "Amplitude", 
				PinInOptions("all values are multiplied by this number")),
			pins::SetupInputPin(PinType::Float, this, &amplitude_shift, 1, "Amplitude_Shift", 
				PinInOptions("is added to all values")),
			pins::SetupInputPin(PinType::Float, this, &phase_shift, 1, "Phase_Shift", 
				PinInOptions("offsets oscillations")),
		};

		PinOutput pin_out_fval = pins::SetupOutputPin(this, pins::PinType::Float, "output");
	};
}