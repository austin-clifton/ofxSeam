#pragma once

#include "seam/include.h"

using namespace seam::pins;

namespace seam::nodes {
	// This will hopefully become unnecessary once Pin Connections are smarter.
	class MultiTrigger : public INode {
	public:
		MultiTrigger();

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		void Update(UpdateParams* params) override;
		
		std::vector<props::NodeProperty> GetProperties() override;

	private:
		void Resize(uint32_t numInputs);

		bool triggered = false;

		uint32_t inputsSize = 1;

		std::vector<PinInput> pinInputs;

		PinOutput pinOutTrigger = pins::SetupOutputPin(this, pins::PinType::Flow, "Trigger");
	};
}