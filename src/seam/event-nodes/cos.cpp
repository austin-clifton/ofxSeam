#include "cos.h"

using namespace seam;

Cos::Cos() : IEventNode("Cosine") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);

	// define output pin
	pin_out_fval.pin = SetupPinOutput(PinType::FLOAT, "output");
}

Cos::~Cos() {

}

PinInput* Cos::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* Cos::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_fval;
}

float Cos::Calculate(float t) {
	// TODO modulating frequency makes this go whacko

	// frequency of 1 == period multiplier of 2PI
	float period_mul = 2.f * PI * pin_frequency.value;
	return pin_amplitude_shift.value + pin_amplitude.value * cos(period_mul * t + pin_phase_shift.value);
}

void Cos::Update(float time) {
	float v = Calculate(time);
	// TODO generalize "propagation" (template function?)
	for (size_t i = 0; i < pin_out_fval.connections.size(); i++) {
		// assert(dynamic_cast<PinFloat*>(pin_out_fval.connections[i].pin) != nullptr);
		static_cast<PinFloat*>(pin_out_fval.connections[i].pin)->value = v;
		pin_out_fval.connections[i].node->SetDirty();
	}
}