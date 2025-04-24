#include "seam/nodes/percussiveTrigger.h"

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
	pinNotesOnStream = FindPinInByName(this, notesOnStreamPinName);
	assert(pinNotesOnStream != nullptr);
}

PercussiveTrigger::~PercussiveTrigger() {
	// NO-OP?
}

void PercussiveTrigger::Retrigger(float velocity, PushPatterns* push) {
		// reset trigger time
		triggerTime = totalTriggerTime;

		// TODO: should this be aware of the last max velocity, in cases where the last trigger hasn't finished?
		triggerMaxVel = velocity;

		// Push to the triggered pin.
		push->PushFlow(pinOutTriggered);
}

void PercussiveTrigger::Update(UpdateParams* params) {
	bool retrigger = eventTriggered;
	float maxVel = retrigger ? 0.8f : 0.f;
	// drain the notes stream;
	// if multiple triggers occurred since the last frame, grab the velocity of the loudest note
	size_t size;
	notes::NoteOnEvent** onEvents = (notes::NoteOnEvent**)pinNotesOnStream->GetEvents(size);

	for (size_t i = 0; i < size; i++) {
		notes::NoteEvent* ev = onEvents[i];
		if (ev->type == notes::EventTypes::ON) {
			notes::NoteOnEvent* on_ev = (notes::NoteOnEvent*)ev;
			maxVel = std::max(maxVel, on_ev->velocity);
			retrigger = true;
		}
	}

	pinNotesOnStream->ClearEvents(size);

	if (retrigger) {
		Retrigger(maxVel, params->push_patterns);
	}

	if (triggerTime > 0.f) {
		float deltaTime = params->delta_time;
		output = CalcTriggerCurve(triggerTime, totalTriggerTime, curveModifier);
		triggerTime = std::max(0.f, triggerTime - deltaTime);
		
		params->push_patterns->Push(pinOutCurve, &output, 1);
	} else if (triggerTime == 0.f) {
		// one last guaranteed trigger to reset to min value
		output = CalcTriggerCurve(triggerTime, totalTriggerTime, curveModifier);
		params->push_patterns->Push(pinOutCurve, &output, 1);
		triggerTime = -1.0f;
	}

	eventTriggered = false;
}

PinInput* PercussiveTrigger::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* PercussiveTrigger::PinOutputs(size_t& size) {
	size = 2;
	return &pinOutCurve;
}