#pragma once

#include "i-node.h"

#include "../pins/pin-float.h"
#include "../pins/pin-flow.h"
#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Adds the input value to the output value every frame.
	class AddStore : public INode {
	public:
		AddStore();
		~AddStore();

		void Update(UpdateParams* params) override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		float value = 0.f;

		PinFloat<1> pin_incrementer = PinFloat<1>(
			"incrementer",
			"will be added to the output value every frame",
			{ 0.0f }
		);

		std::array<IPinInput*, 1> pin_inputs = {
			&pin_incrementer
		};

		PinOutput pin_out_value = pins::SetupOutputPin(this, pins::PinType::FLOAT, "value");
	};
}