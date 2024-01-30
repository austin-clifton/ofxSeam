#include "seam/nodes/gate.h"

using namespace seam;
using namespace seam::nodes;

Gate::Gate() : INode("Gate") {
	pinInputs.push_back(SetupInputPin(PinType::INT, this, &selectedGate, 1, "Selected Gate"));
	UpdateGatesCount(2);
}

Gate::~Gate() {

}

void Gate::Update(UpdateParams* params) {
	size_t valueIndex = selectedGate * numCoords;
	if (gatedValues.size() > valueIndex) {
		params->push_patterns->Push(pinOutSelection, &gatedValues[valueIndex], 1);
	}

	params->push_patterns->PushFlow(pinOutGateChangedEvent);
}

pins::PinInput* Gate::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

pins::PinOutput* Gate::PinOutputs(size_t& size) {
	size = 2;
	return &pinOutSelection;
}

bool Gate::GuiDrawPropertiesList(UpdateParams* params) {
	bool changed = false;

	uint32_t newGatesCount = gatesCount;
	if (ImGui::InputScalar("Gates Count", ImGuiDataType_U32, &newGatesCount)) {
		UpdateGatesCount(newGatesCount);
		changed = true;
 	}

	uint16_t newNumCoords = pinOutSelection.NumCoords();
	if (ImGui::InputScalar("Num Coords", ImGuiDataType_U16, &newNumCoords)) {
		UpdateNumCoords(newNumCoords);
		changed = true;
	}

	return changed;
}

std::vector<props::NodeProperty> Gate::GetProperties() {
	using namespace seam::props;

	std::vector<NodeProperty> props;

	props.push_back(SetupUintProperty("Gates Count", [this](size_t& size) {
		size = 1;
		return &gatesCount;
	}, [this](uint32_t* newCount, size_t size) {
		assert(size == 1);
		UpdateGatesCount(*newCount);
	}));

	props.push_back(SetupUintProperty("Num Coords", [this](size_t& size) {
		size = 1;
		return &numCoords;
	}, [this](uint32_t* newNumCoords, size_t size) {
		assert(size == 1);
		UpdateNumCoords((uint16_t)(*newNumCoords));
	}));

	return props;
}

void Gate::UpdateGatesCount(uint32_t newGatesCount) {
	newGatesCount = std::min(2U, newGatesCount);
	if (newGatesCount > gatesCount) {
		while (gatesCount < newGatesCount) {
			AddGatePin("Gate " + std::to_string(gatesCount));
		}
	} else {
		// Add one since the first input pin is the select line.
		pinInputs.resize(newGatesCount + 1);
	}

	RecacheInputConnections();

	// Minus one here for the same reason we added one above (account for select line input pin)
	gatesCount = pinInputs.size() - 1;
	assert(gatesCount == newGatesCount);
}

void Gate::UpdateNumCoords(uint16_t newNumCoords) {
	numCoords =	newNumCoords;
	pinOutSelection.SetNumCoords(newNumCoords);
	ResizeGatedValues();
}

void Gate::ResizeGatedValues() {
	// TODO ideally this would preserve any already-set-up values...
	gatedValues.resize(numCoords * gatesCount, 0.f);

	// Fix pin input pointers to gated values.
	for (size_t i = 1; i < pinInputs.size(); i += 1) {
		// Technically this call only needs to be done in UpdateNumCoords(),
		// but this is a little simpler.
		pinInputs[i].SetNumCoords(numCoords);

		pinInputs[i].SetBuffer(&gatedValues[(i - 1) * numCoords], 1);
	}
}

PinInput* Gate::AddGatePin(const std::string_view name) {
	gatesCount += 1;
	ResizeGatedValues();
	pinInputs.push_back(SetupInputPin(PinType::FLOAT, this, &gatedValues[gatedValues.size() - numCoords], 1, name));
	return &pinInputs[pinInputs.size() - 1];
}