#pragma once

#include "i-node.h"

#include "../pins/pin-float.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Saw wave signal generator
	class Saw : public INode {
	public:
		Saw();
		~Saw();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		float progress = 0.f;

		float frequency = 1.f;
		float leadingEdge = 1.f;
		float fallingEdge = 0.01f;

		/*
		PinFloat<1> pin_frequency = PinFloat<1>(
			"frequency", 
			"a frequency of one will oscillate once per second; two will oscillate twice per second, etc.",
			{ 1.f },
			0.000001f
		);
		*/
		// PinFloat<1> pin_leading_edge = PinFloat<1>("leading edge", "right before the saw wave snap, the value will be this", { 1.f });
		// PinFloat<1> pin_falling_edge = PinFloat<1>("falling edge", "right after the saw wave snap, the value will be this", { 0.01f });

		std::array<PinInput, 3> pin_inputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &frequency, 1, "Frequency"),
			pins::SetupInputPin(PinType::FLOAT, this, &leadingEdge, 1, "Leading Edge", sizeof(float), nullptr,  "right before the saw wave snap, the value will be this"),
			pins::SetupInputPin(PinType::FLOAT, this, &fallingEdge, 1, "Falling Edge", sizeof(float), nullptr, "right after the saw wave snap, the value will be this"),
		};

		PinOutput pin_out_fval = pins::SetupOutputPin(this, pins::PinType::FLOAT, "output");
	};
}