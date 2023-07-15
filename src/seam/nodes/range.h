#pragma once

#include "i-node.h"

#include "../pins/pin-float.h"
#include "../pins/pin-flow.h"
#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Adds the input value to the output value every frame.
	class Range : public INode {
	public:
		Range();
		~Range();

		void Update(UpdateParams* params) override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		float value = 0.f;

		PinFloat<1> pin_input_range_min = PinFloat<1>("input min", "", { -1.0f });
		PinFloat<1> pin_input_range_max = PinFloat<1>("input max", "", { 1.0f });
		PinFloat<1> pin_input_value = PinFloat<1>("input value", "", { 0.0f });
		PinFloat<1> pin_output_range_min = PinFloat<1>("output min", "", { 0.0f });
		PinFloat<1> pin_output_range_max = PinFloat<1>("output max", "", { 1.0f });

		std::array<IPinInput*, 5> pin_inputs = {
			&pin_input_range_min,
			&pin_input_range_max,
			&pin_input_value,
			&pin_output_range_min,
			&pin_output_range_max
		};

		PinOutput pin_out_value = pins::SetupOutputPin(this, pins::PinType::FLOAT, "value");
	};
}