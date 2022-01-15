#include "cos.h"

using namespace seam;
using namespace seam::nodes;

Cos::Cos() : INode("Cosine") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);

	// define output pin
	pin_out_fval.pin = pins::SetupOutputPin(this, pins::PinType::FLOAT, "output");
}

Cos::~Cos() {

}

pins::IPinInput** Cos::PinInputs(size_t& size) {
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
	float period_mul = 2.f * PI * pin_frequency[0];
	return pin_amplitude_shift[0] + pin_amplitude[0] * cos(period_mul * t + pin_phase_shift[0]);
}

void Cos::Update(UpdateParams* params) {
	float v = Calculate(params->time);
	// TODO generalize "propagation" (template function?)
	for (size_t i = 0; i < pin_out_fval.connections.size(); i++) {
		size_t chans_size;
		float* channels = (float*)pin_out_fval.connections[i]->GetChannels(chans_size);

		params->push_patterns->Push(pin_out_fval, &v, 1);
	}
}