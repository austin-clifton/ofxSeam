#pragma once

#include "iNode.h"

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
		const char* notesOnStreamPinName = "Notes On Stream";

		void Retrigger(float velocity, PushPatterns* push);

		bool eventTriggered = false;

		float triggerTime = 0.f;
		float triggerMaxVel = 0.f;
		float output = 0.f;

		float curveModifier = -4.0f;
		float totalTriggerTime = 0.4f;
		PinInput* pinNotesOnStream;

		std::array<PinInput, 4> pin_inputs = {
			pins::SetupInputFlowPin(this, [this] { eventTriggered = true; }, "Trigger"),
			pins::SetupInputQueuePin(PinType::NOTE_EVENT, this, notesOnStreamPinName),
			pins::SetupInputPin(PinType::FLOAT, this, &curveModifier, 1, "Curve Modifier", 
				PinInOptions("0 for linear, positive for an x^N curve, negative for x^(1 / -N) curve")),
			pins::SetupInputPin(PinType::FLOAT, this, &totalTriggerTime, 1, "Total Trigger Time", 
				PinInOptions("the total amount of time each trigger animation runs for, in seconds")),
		};

		PinOutput pinOutCurve = pins::SetupOutputPin(this, pins::PinType::FLOAT, "Trigger Curve");
		PinOutput pinOutTriggered = pins::SetupOutputPin(this, pins::PinType::FLOW, "On Triggered");
	};
}