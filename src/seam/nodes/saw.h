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

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		PinFloat<1> pin_frequency = PinFloat<1>(
			"frequency", 
			"a frequency of one will oscillate once per second; two will oscillate twice per second, etc.",
			{ 1.f },
			0.000001f
		);

		PinFloat<1> pin_leading_edge = PinFloat<1>("leading edge", "right before the saw wave snap, the value will be this", { 1.f });
		PinFloat<1> pin_falling_edge = PinFloat<1>("falling edge", "right after the saw wave snap, the value will be this", { 0.01f });

		std::array<IPinInput*, 3> pin_inputs = {
			&pin_frequency,
			&pin_leading_edge,
			&pin_falling_edge,
		};

		PinOutput pin_out_fval = pins::SetupOutputPin(this, pins::PinType::FLOAT, "output");

		float progress = 0.f;
	};
}