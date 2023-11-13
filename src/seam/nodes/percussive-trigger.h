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

		bool GuiDrawPropertiesList(UpdateParams* params) override;

	private:
		void Retrigger(float velocity, PushPatterns* push);

		float trigger_time = 0.f;
		float trigger_max_vel = 0.f;
		float output = 0.f;

		float curveModifier = 4.0f;
		float totalTriggerTime = 0.4f;
		PinInput* pinNotesOnStream;

		std::array<PinInput, 3> pin_inputs = { 
			pins::SetupInputQueuePin(PinType::NOTE_EVENT, this, "Notes On Stream"),
			pins::SetupInputPin(PinType::FLOAT, this, &curveModifier, 1, "Curve Modifier", 
				PinInOptions("0 for linear, positive for an x^N curve, negative for x^(1 / -N) curve")),
			pins::SetupInputPin(PinType::FLOAT, this, &totalTriggerTime, 1, "Total Trigger Time", 
				PinInOptions("the total amount of time each trigger animation runs for, in seconds")),
		};

		PinOutput pinOutCurve = pins::SetupOutputPin(this, pins::PinType::FLOAT, "Trigger Curve");
		PinOutput pinOutTriggered = pins::SetupOutputPin(this, pins::PinType::FLOW, "On Triggered");
	};
}