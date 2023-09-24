#pragma once

#include "i-node.h"

#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Adds the input value to the output value every frame.
	class Range : public INode {
	public:
		Range();
		~Range();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		float value = 0.f;

		float inRangeMin = -1.0f;
		float inRangeMax = 1.0f;
		float inValue = 0.f;
		float outRangeMin = 0.f;
		float outRangeMax = 1.f;

		std::array<PinInput, 5> pin_inputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &inRangeMin, 1, "Input Range Min"),
			pins::SetupInputPin(PinType::FLOAT, this, &inRangeMax, 1, "Input Range Max"),
			pins::SetupInputPin(PinType::FLOAT, this, &inValue, 1, "Input Value"),
			pins::SetupInputPin(PinType::FLOAT, this, &outRangeMin, 1, "Output Range Min"),
			pins::SetupInputPin(PinType::FLOAT, this, &outRangeMax, 1, "Output Range Min"),
		};

		PinOutput pin_out_value = pins::SetupOutputPin(this, pins::PinType::FLOAT, "value");
	};
}