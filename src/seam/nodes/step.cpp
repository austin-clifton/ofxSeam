#include "seam/nodes/step.h"

using namespace seam;
using namespace seam::nodes;

Step::Step() : INode("Step") {
	// temp???@@@
	flags = (NodeFlags)(flags | NodeFlags::UpdatesOverTime);
}

Step::~Step() {

}

pins::PinInput* Step::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

pins::PinOutput* Step::PinOutputs(size_t& size) {
	size = 2;
	return &pin_out_fval;
}

void Step::Update(UpdateParams* params) {
	bool output;
	if (comparator < 0.f) {
		output = input < edge;
	}
	else if (comparator > 0.f) {
		output = input > edge;
	}
	else {
		output = abs(input - edge) < FLT_EPSILON;
	}

	float fout = (float)output;

	params->push_patterns->Push(pin_out_fval, &fout, 1);
	params->push_patterns->Push(pin_out_bval, &output, 1);
}