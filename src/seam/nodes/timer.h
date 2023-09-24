#pragma once

#include "i-node.h"

#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// Output increments over time multiplied by speed until reset with the trigger.
	/// Can also be paused.
	class Timer : public INode {
	public:
		Timer();
		~Timer();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		void Pause() {
			time = 0.f;
		}

		float time = 0.f;
		float speed = 1.f;
		bool pause = false;

		/*
		PinBool<1> pin_pause = PinBool<1>(
			"pause", 
			"pause the timer", 
			{ false }
		);

		PinFlow pin_reset = PinFlow(
			std::function<void(void)>([&] { time = 0.f; }),
			"reset", 
			"reset the timer"
		);

		PinFloat<1> pin_speed = PinFloat<1>(
			"speed", 
			"rate of output increment for the timer", 
			{ 1.f }
		);
		*/

		std::array<PinInput, 3> pin_inputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &time, 1, "Time"),
			pins::SetupInputPin(PinType::FLOAT, this, &speed, 1, "Speed"),
			pins::SetupInputFlowPin(this,[&]() { Pause(); }, "Reset")
		};

		PinOutput pin_out_time = pins::SetupOutputPin(this, pins::PinType::FLOAT, "time");
		PinOutput pin_out_delta_time = pins::SetupOutputPin(this, pins::PinType::FLOAT, "delta time");
	};
}