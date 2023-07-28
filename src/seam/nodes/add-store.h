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

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		float inc = 0.f;
		float value = 0.f;

		std::array<PinInput, 1> pin_inputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &inc, 1,
				"incrementer", sizeof(float), nullptr, "will be added to the output value every frame"),
		};

		PinOutput pin_out_value = pins::SetupOutputPin(this, pins::PinType::FLOAT, "value");
	};
}