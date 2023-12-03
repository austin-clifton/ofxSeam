#include "gate.h"

using namespace seam;
using namespace seam::nodes;

Gate::Gate() : IDynamicPinsNode("Gate") {
	// bad hack 2 pls removeme
	// flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);

	// !! BAD HACK !!:
	// Reserve space for pins so the pins list pointer doesn't change when de-serializing.
	// This needs to be fixed in deserialization!!!!!!
	pinInputs.reserve(8);
	gatedValues.reserve(8);
	pinInputs.push_back(SetupInputPin(PinType::INT, this, &selectedGate, 1, "Selected Gate"));
}

Gate::~Gate() {

}

void Gate::Update(UpdateParams* params) {
	if (gatedValues.size() > selectedGate) {
		params->push_patterns->Push(pinOutSelection, &gatedValues[selectedGate], 1);
	}

	// params->push_patterns->PushFlow(pinOutGateChangedEvent);
}

pins::PinInput* Gate::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

pins::PinOutput* Gate::PinOutputs(size_t& size) {
	size = 2;
	return &pinOutSelection;
}

pins::PinInput* Gate::AddPinIn(PinInArgs args) {
	// Expect gate value input pins.
	assert(args.type == PinType::FLOAT && args.totalElements == 1);
	return AddGatePin(args.name);
}

pins::PinOutput* Gate::AddPinOut(PinOutput&& pinOut, size_t index) {
	// No dynamic outputs expected.
	return nullptr;
}

bool Gate::GuiDrawPropertiesList(UpdateParams* params) {
	uint32_t newGatesCount = gatesCount;
	if (ImGui::InputScalar("Gates Count", ImGuiDataType_U32, &newGatesCount)) {
		if (newGatesCount > gatesCount) {
			while (gatesCount < newGatesCount) {
				AddGatePin("Gate " + std::to_string(gatesCount));
			}
			return true;
		} else {
			// OH NOOO, TODOOOOOO
		}
 	}
	return false;
}

PinInput* Gate::AddGatePin(const std::string_view name) {
	gatesCount += 1;
	gatedValues.push_back((float)gatedValues.size());
	pinInputs.push_back(SetupInputPin(PinType::FLOAT, this, &gatedValues[gatedValues.size() - 1], 1, name));
	return &pinInputs[pinInputs.size() - 1];
}