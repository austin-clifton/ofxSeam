
#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Picks a random value in the input range when triggered.
	class Random : public INode {
	public:
		Random();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		std::vector<props::NodeProperty> GetProperties() override;

	private:
        void CalculateOutput();

        glm::vec2 range = glm::vec2(0.f, 1.f);

        glm::vec4 value = glm::vec4(0.f);

        uint32_t numCoords = 1;

		std::array<PinInput, 3> pinInputs = {
			pins::SetupInputPin(PinType::Float, this, &range[0], 1, "Random Min"),
			pins::SetupInputPin(PinType::Float, this, &range[1], 1, "Random Max"),
            pins::SetupInputFlowPin(this, std::bind(&Random::CalculateOutput, this),
                "Recalculate Value")
		};

		PinOutput pinOutValue = pins::SetupOutputPin(this, pins::PinType::Float, "Rand Out");
	};
}