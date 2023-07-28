#pragma once

#include "i-node.h"
#include "../pins/pin-note-event.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Converts note events into percussive curve signals.
	/// Can turn one-shot percussive events into a suitable animation curve.
	class PercussiveTrigger : public INode {
	public:
		PercussiveTrigger();
		virtual ~PercussiveTrigger();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		float trigger_time = 0.f;
		float trigger_max_vel = 0.f;
		float output = 0.f;

		/*
		PinNoteEvent<0> pin_notes_stream = PinNoteEvent<0>(
			"notes stream",
			"incoming stream of percussive notes",
			notes::EventTypes::ON
		);

		PinFloat<1> pin_delta_time = PinFloat<1>(
			"time",
			"optional time modifier",
			{ 0.f }
		);

		PinFloat<1> pin_curve_modifier = PinFloat<1>(
			"curve modifier",
			"0 for linear, positive for an x^N curve, negative for x^(1 / -N) curve",
			{ -2.f }
		);

		PinFloat<1> pin_total_trigger_time = PinFloat<1>(
			"total trigger time",
			"the total amount of time each trigger animation runs for, in seconds",
			{ 0.25f },
			{ 0.f }
		);

		std::array<IPinInput*, 4> pin_inputs = {
			&pin_notes_stream,
			&pin_delta_time,
			&pin_curve_modifier,
			&pin_total_trigger_time
		};
		*/

		float curveModifier;
		float totalTriggerTime;
		PinInput* pinNotesOnStream;

		std::array<PinInput, 3> pin_inputs = { 
			pins::SetupInputQueuePin(PinType::NOTE_EVENT, this, "Notes On Stream"),
			pins::SetupInputPin(PinType::FLOAT, this, &curveModifier, 1, "Curve Modifier", sizeof(float), nullptr, 
				"0 for linear, positive for an x^N curve, negative for x^(1 / -N) curve"),
			pins::SetupInputPin(PinType::FLOAT, this, &totalTriggerTime, 1, "Total Trigger Time", sizeof(float), nullptr, 
				"the total amount of time each trigger animation runs for, in seconds"),
		};

		PinOutput pin_out_curve = pins::SetupOutputPin(this, pins::PinType::FLOAT, "trigger curve");
	};
}