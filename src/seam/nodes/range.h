#pragma once

#include "iNode.h"

#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Adds the input value to the output value every frame.
	class Range : public INode {
	public:
		Range();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		void OnPinConnected(PinConnectedArgs args) override;

	private:
		const static char* inputValuePinName;

		glm::vec4 inRangeMin = glm::vec4(-1.0f);
		glm::vec4 inRangeMax = glm::vec4(1.0f);
		glm::vec4 inValue = glm::vec4(0.f);
		glm::vec4 outRangeMin = glm::vec4(0.f);
		glm::vec4 outRangeMax = glm::vec4(1.f);

		std::array<PinInput, 5> pinInputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &inRangeMin, 1, "Input Range Min"),
			pins::SetupInputPin(PinType::FLOAT, this, &inRangeMax, 1, "Input Range Max"),
			pins::SetupInputPin(PinType::FLOAT, this, &inValue, 1, inputValuePinName),
			pins::SetupInputPin(PinType::FLOAT, this, &outRangeMin, 1, "Output Range Min"),
			pins::SetupInputPin(PinType::FLOAT, this, &outRangeMax, 1, "Output Range Max"),
		};

		PinOutput pin_out_value = pins::SetupOutputPin(this, pins::PinType::FLOAT, "value");
	};
}