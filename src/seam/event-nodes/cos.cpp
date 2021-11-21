#include "cos.h"

using namespace seam;

Cos::Cos() : IEventNode("Cosine") {
	// define input pins
	pin_inputs[0] = CreatePinInput<float>("frequency", [&](float v) {
		frequency = v;
	});

	pin_inputs[1] = CreatePinInput<float>("amplitude", [&](float v) {
		amplitude = v;
	});

	pin_inputs[2] = CreatePinInput<float>("amplitude shift", [&](float v) {
		amplitude_shift = v;
	});

	pin_inputs[3] = CreatePinInput<float>("phase_shift", [&](float v) {
		phase_shift = v;
	});

	// define output pin
	pin_out_fval.pin = CreatePinOutput(PinType::FLOAT, "output");
}

Cos::~Cos() {
	// delete input pins
	for (size_t i = 0; i < pin_inputs.size(); i++) {
		delete pin_inputs[i].pin;
		pin_inputs[i].pin = nullptr;
	}
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
	// frequency of 1 == period multiplier of 2PI
	float period_mul = 2.f * PI * frequency;
	return amplitude_shift + amplitude * cos(period_mul * t + phase_shift);
}

void Cos::Update(float time) {
	float v = Calculate(time);
	for (size_t i = 0; i < pin_out_fval.connections.size(); i++) {
		static_cast<PinFloat*>(pin_out_fval.connections[i])->callback(v);
	}
}