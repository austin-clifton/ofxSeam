#include "seam/nodes/cos.h"

using namespace seam;
using namespace seam::nodes;

Cos::Cos() : INode("Cosine") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);
}

Cos::~Cos() {

}

pins::PinInput* Cos::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

pins::PinOutput* Cos::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_fval;
}

float Cos::Calculate(float t) {
	// TODO modulating frequency makes this go whacko

	// frequency of 1 == period multiplier of 2PI
	float period_mul = 2.f * PI * frequency;
	return amplitude_shift + amplitude * cos(period_mul * t + phase_shift);
}

void Cos::Update(UpdateParams* params) {
	float v = Calculate(params->time);
	// TODO generalize "propagation" (template function?)
	for (size_t i = 0; i < pin_out_fval.connections.size(); i++) {
		params->push_patterns->Push(pin_out_fval, &v, 1);
	}
}