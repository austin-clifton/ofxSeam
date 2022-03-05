#include "timer.h"

using namespace seam;
using namespace seam::nodes;

Timer::Timer() : INode("Timer") {
	// TODO you may want to instead update every frame;
	// I could see this causing some weirdness later on...
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);
}

Timer::~Timer() {

}

IPinInput** Timer::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return &pin_inputs[0];
}

PinOutput* Timer::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_time;
}

void Timer::Update(UpdateParams* params) {
	// only update if we're not paused
	if (!pin_pause[0]) {
		time += params->delta_time * pin_speed[0];

		params->push_patterns->Push(pin_out_time, &time, 1);
	}
}