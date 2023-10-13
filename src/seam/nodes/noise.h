#pragma once

#include "iNode.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Noise wave signal generator
	class Noise : public INode {
	public:
		Noise();
		~Noise();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		float sample = 0.f;

		float speedMultiplier = 1.f;
		float minN = 0.f;
		float maxN = 1.f;

		std::array<PinInput, 3> pin_inputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &speedMultiplier, 1, "Speed"),
			pins::SetupInputPin(PinType::FLOAT, this, &minN, 1, "Min Output"),
			pins::SetupInputPin(PinType::FLOAT, this, &maxN, 1, "Max Output"),

		};

		PinOutput pinOutNoiseVal = pins::SetupOutputPin(this, pins::PinType::FLOAT, "Output");
	};
}