#include "gate.h"

using namespace seam;
using namespace seam::nodes;

Gate::Gate() : IDynamicPinsNode("Gate") {
	// !! BAD HACK !!:
	// Reserve space for pins so the pins list pointer doesn't change when de-serializing.
	// This needs to be fixed in deserialization!!!!!!
	pinInputs.reserve(8);
	pinInputs.push_back(SetupInputPin(PinType::INT, this, &selectedGate, 1, "Selected Gate"));
}

Gate::~Gate() {

}

void Gate::Update(UpdateParams* params) {
	params->push_patterns->Push(pinOutSelection, &gatedValues[selectedGate], 1);
}

pins::PinInput* Gate::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

pins::PinOutput* Gate::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutSelection;
}

pins::PinInput* Gate::AddPinIn(PinType type, const std::string_view name, size_t elementSize, size_t elementCount) {
	// Expect gate value input pins.
	assert(type == PinType::FLOAT && elementCount == 1 && elementSize == sizeof(float));
	return AddGatePin(name);
}

pins::PinOutput* Gate::AddPinOut(PinOutput&& pinOut) {
	// No dynamic outputs expected.
	return nullptr;
}

bool Gate::GuiDrawPropertiesList() {
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