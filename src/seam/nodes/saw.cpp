#include "saw.h"

using namespace seam;
using namespace seam::nodes;

Saw::Saw() : INode("Saw") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);
}

Saw::~Saw() {

}

pins::IPinInput** Saw::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

pins::PinOutput* Saw::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_fval;
}

void Saw::Update(UpdateParams* params) {
	progress += (params->delta_time) / pin_frequency[0];
	progress = fmod(progress, 1.0f);

	float v = pin_leading_edge[0] + progress * (pin_falling_edge[0] - pin_leading_edge[0]);

	// TODO generalize "propagation" (template function?)
	for (size_t i = 0; i < pin_out_fval.connections.size(); i++) {
		size_t chans_size;
		float* channels = (float*)pin_out_fval.connections[i]->GetChannels(chans_size);

		params->push_patterns->Push(pin_out_fval, &v, 1);
	}
}