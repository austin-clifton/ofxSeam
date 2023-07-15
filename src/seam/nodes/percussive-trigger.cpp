#include "percussive-trigger.h"

using namespace seam;
using namespace seam::nodes;

namespace {
	// TODO this probably belongs elsewhere
	/// \param time_remaining should be less than or equal to total_time, and greater than 0, for valid results.
	/// \param total_time the total duration of the animation curve in seconds. Should be greater than time_remaining and greater than 0
	/// \param 0 for linear, positive for an x^N curve, negative for x^(1 / -N) curve
	/// \returns [0..1] progress along the curve given the input values
	float CalcTriggerCurve(float time_remaining, float total_time, float curve_modifier) {
		assert(total_time != 0.f);
		float normalized_time = 1.f - (time_remaining / total_time);
		
		// power is 0 for a linear curve
		float power = 0.f;
		// the ranges [-1..0) and (0..1] produce duplicate results here.
		// not sure I really care? 
		if (curve_modifier > 0.f) {
			power = curve_modifier;
		} else if (curve_modifier < 0.f) {
			power = (1.f / -curve_modifier);
		}

		return 1.0 - std::pow(normalized_time, power);
	}
}

PercussiveTrigger::PercussiveTrigger() : INode("Percussive Trigger") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);
}

PercussiveTrigger::~PercussiveTrigger() {
	// NO-OP?
}

void PercussiveTrigger::Update(UpdateParams* params) {
	float max_vel = 0.f;
	bool retrigger = false;
	// drain the notes stream;
	// if multiple triggers occurred since the last frame, grab the velocity of the loudest note
	for (size_t i = pin_notes_stream.channels.size(); i > 0; i--) {
		notes::NoteEvent* ev = pin_notes_stream[i - 1];
		if (ev->type == notes::EventTypes::ON) {
			notes::NoteOnEvent* on_ev = (notes::NoteOnEvent*)ev;
			max_vel = std::max(max_vel, on_ev->velocity);
			retrigger = true;
		}
		
		pin_notes_stream.channels.pop_back();
	}

	if (retrigger) {
		// reset trigger time
		trigger_time = pin_total_trigger_time[0];

		// TODO: should this be aware of the last max velocity, in cases where the last trigger hasn't finished?
		trigger_max_vel = max_vel;
	}

	if (trigger_time > 0.f) {
		// use time input as override if present
		float delta_time = pin_delta_time.connection != nullptr
			? pin_delta_time[0]
			: params->delta_time;


		output = CalcTriggerCurve(trigger_time, pin_total_trigger_time[0], pin_curve_modifier[0]);
		trigger_time = std::max(0.f, trigger_time - delta_time);
		
		params->push_patterns->Push(pin_out_curve, &output, 1);
	} else if (trigger_time == 0.f) {
		// one last guaranteed trigger to reset to min value
		output = CalcTriggerCurve(trigger_time, pin_total_trigger_time[0], pin_curve_modifier[0]);
		params->push_patterns->Push(pin_out_curve, &output, 1);
		trigger_time = -1.0f;
	}
}

IPinInput** PercussiveTrigger::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* PercussiveTrigger::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_curve;
}