#pragma once

#include "seam/include.h"

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
		void Reset();

		float progress = 0.f;

		float frequency = 1.f;
		float leadingEdge = 1.f;
		float fallingEdge = 0.01f;

		std::array<PinInput, 4> pin_inputs = {
			pins::SetupInputPin(PinType::Float, this, &frequency, 1, "Frequency"),
			pins::SetupInputPin(PinType::Float, this, &leadingEdge, 1, "Leading Edge", 
				PinInOptions("right before the saw wave snap, the value will be this")),
			pins::SetupInputPin(PinType::Float, this, &fallingEdge, 1, "Falling Edge", 
				PinInOptions("right after the saw wave snap, the value will be this")),
			pins::SetupInputFlowPin(this, [&] { Reset(); }, "Reset"),
		};

		PinOutput pin_out_fval = pins::SetupOutputPin(this, pins::PinType::Float, "output");
	};
}