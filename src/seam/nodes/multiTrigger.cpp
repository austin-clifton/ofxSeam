#include "multiTrigger.h"

using namespace seam;
using namespace seam::nodes;

MultiTrigger::MultiTrigger() : INode("Multi Trigger") {
	Resize(inputsSize);
}

pins::PinInput* MultiTrigger::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

pins::PinOutput* MultiTrigger::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutTrigger;
}

void MultiTrigger::Update(UpdateParams* params) {
	if (triggered) {
		params->push_patterns->PushFlow(pinOutTrigger);
		triggered = false;
	}
}

std::vector<props::NodeProperty> MultiTrigger::GetProperties() {
	std::vector<props::NodeProperty> props;

	props.push_back(props::SetupUintProperty("Num Inputs", 
		[this](size_t& size) {
			size = 1;
			return &inputsSize;
		}, [this](uint32_t* newSize, size_t size) {
			assert(size == 1);
			uint32_t actualNewSize = std::min(*newSize, 4U);
			Resize(actualNewSize);
		})
	);

	return props;
}

void MultiTrigger::Resize(uint32_t newNumInputs) {
	while (pinInputs.size() < newNumInputs) {
		std::string name = "Input " + std::to_string(pinInputs.size());

		pinInputs.push_back(SetupInputFlowPin(this, [this] {
			triggered = true;
		}, name));
	}

	if (pinInputs.size() > newNumInputs) {
		pinInputs.resize(newNumInputs);
	}

	RecacheInputConnections();

	inputsSize = newNumInputs;
}