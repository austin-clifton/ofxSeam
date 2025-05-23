#pragma once

#include "seam/include.h"

using namespace seam::pins;

namespace seam::nodes {
	/// <summary>
	/// Output 0 or 1 depending on the input and edge. This is meant to be like GLSL's step() in some ways.
	/// TODO: ideally float/int/bool should all convert nicely between each other in the pin system somehow.
	/// </summary>
	class Step : public INode {
	public:
		Step();
		~Step();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;
	private:
		float edge = 1.f;	
		float comparator = 0.f;
		float input = 0.f;

		std::array<PinInput, 3> pin_inputs = {
			pins::SetupInputPin(PinType::Float, this, &edge, 1, "Edge", 
				PinInOptions("The input value will be compared to this edge value using the comparator.")),
			pins::SetupInputPin(PinType::Float, this, &comparator, 1, "Comparator", 
				PinInOptions("When < 0, output is input < edge, when == 0 equality is used, when > 0 check input > edge" )),
			pins::SetupInputPin(PinType::Float, this, &input, 1, "Input"),
		};

		PinOutput pin_out_fval = pins::SetupOutputPin(this, pins::PinType::Float, "Float Output");
		PinOutput pin_out_bval = pins::SetupOutputPin(this, pins::PinType::Bool, "Bool Output");
	};
}