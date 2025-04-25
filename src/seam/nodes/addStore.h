#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

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
			pins::SetupInputPin(PinType::Float, this, &inc, 1, "incrementer", 
				pins::PinInOptions("will be added to the output value every frame")),	
		};

		PinOutput pin_out_value = pins::SetupOutputPin(this, pins::PinType::Float, "value");
	};
}