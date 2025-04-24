#include "seam/nodes/saw.h"

using namespace seam;
using namespace seam::nodes;

Saw::Saw() : INode("Saw") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);
}

Saw::~Saw() {

}

pins::PinInput* Saw::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

pins::PinOutput* Saw::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_fval;
}

void Saw::Update(UpdateParams* params) {
	progress += (params->delta_time) / frequency;
	progress = fmod(progress, 1.0f);

	float v = leadingEdge + progress * (fallingEdge - leadingEdge);

	// TODO generalize "propagation" (template function?)
	for (size_t i = 0; i < pin_out_fval.connections.size(); i++) {
		params->push_patterns->Push(pin_out_fval, &v, 1);
	}
}

void Saw::Reset() {
	progress = 0.f;
}