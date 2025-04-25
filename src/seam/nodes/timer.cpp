#include "seam/nodes/timer.h"

using namespace seam;
using namespace seam::nodes;

Timer::Timer() : INode("Timer") {
	// TODO you may want to instead update every frame;
	// I could see this causing some weirdness later on...
	flags = (NodeFlags)(flags | NodeFlags::UpdatesOverTime);
}

Timer::~Timer() {

}

PinInput* Timer::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return &pin_inputs[0];
}

PinOutput* Timer::PinOutputs(size_t& size) {
	// DANGER ZONE? there ARE two pin outputs, declared next to each other in the class.
	// so this should be safe...?
	size = 2;
	return &pin_out_time;
}

void Timer::Update(UpdateParams* params) {
	// only update if we're not paused
	if (!pause) {
		time += params->delta_time * speed;

		params->push_patterns->Push(pin_out_time, &time, 1);
	}
}