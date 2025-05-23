#include "seam/nodes/select.h"
#include "seam/pins/pin.h"

using namespace seam;
using namespace seam::nodes;

Select::Select() : IDynamicPinsNode("Select") {
	 
}

Select::~Select() {

}

void Select::Setup(SetupParams* params) {
	pinInputs.push_back(SetupInputPin(PinType::Int, this, &selectedGate, 1, "Selected Gate"));
	UpdateGatesCount(2);
}

void Select::Update(UpdateParams* params) {
	selectedGate = std::min(selectedGate, gatesCount - 1);
	size_t valueIndex = selectedGate * numCoords * pins::PinTypeToElementSize(inputPinType);
	params->push_patterns->Push(pinOutSelection, &gatedValues[valueIndex], 1);

	if (lastSelectedGate != selectedGate) {
		params->push_patterns->PushFlow(pinOutGateChangedEvent);
		lastSelectedGate = selectedGate;
		
		// Update disabled flag on input pins
		for (size_t i = 1; i < pinInputs.size(); i++) {
			pinInputs[i].SetEnabled((i - 1) == selectedGate);
		}
	}
}

pins::PinInput* Select::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

pins::PinOutput* Select::PinOutputs(size_t& size) {
	size = 2;
	return &pinOutSelection;
}

bool Select::GuiDrawPropertiesList(UpdateParams* params) {
	bool changed = false;

	uint32_t newGatesCount = gatesCount;
	if (ImGui::InputScalar("Selections Count", ImGuiDataType_U32, &newGatesCount)) {
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

std::vector<props::NodeProperty> Select::GetProperties() {
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

pins::PinInput* Select::AddPinIn(PinInArgs args) {
	inputPinType = args.type;
	assert(pinInputs.size() == args.index);
	assert(args.totalElements == 1);
	return AddGatePin(args.name);
}

void Select::UpdateGatesCount(uint32_t newGatesCount) {
	newGatesCount = std::max(2U, newGatesCount);
	if (newGatesCount > gatesCount) {
		while (gatesCount < newGatesCount) {
			AddGatePin("Selection " + std::to_string(gatesCount));
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

void Select::UpdateNumCoords(uint16_t newNumCoords) {
	numCoords =	newNumCoords;
	pinOutSelection.SetNumCoords(newNumCoords);
	ResizeGatedValues();
}

void Select::ResizeGatedValues() {
	// Nothing to do until the pin type is known.
	if (inputPinType == PinType::Any) {
		return;
	}

	size_t bytesPerValue = pins::PinTypeToElementSize(inputPinType);
	gatedValues.resize(numCoords * gatesCount * bytesPerValue, std::byte(0));

	// Fix pin input pointers to gated values.
	for (size_t i = 1; i < pinInputs.size(); i += 1) {
		// Technically this call only needs to be done in UpdateNumCoords(),
		// but this is a little simpler.
		pinInputs[i].SetNumCoords(numCoords);
		pinInputs[i].SetBuffer(&gatedValues[(i - 1) * numCoords * bytesPerValue], 1);
	}
}

PinInput* Select::AddGatePin(const std::string_view name) {
	pins::ConnectedCallback connectCb;
	// Until the user has connected the first input pin, we don't know what type inputs should be.
	if (inputPinType == PinType::Any) {
		connectCb = [this](PinConnectedArgs args) {
			// After the first connection sets the gate pins' types, 
			// we just bail out here. Nothing to do.
			if (args.pinIn->Type() != PinType::Any) {
				return;
			}

			inputPinType = args.pinOut->Type();
			for (size_t i = 1; i < pinInputs.size(); i++) {
				PinInput& gatePin = pinInputs[i];
				gatePin.SetType(inputPinType);
			}

			pinOutSelection.SetType(inputPinType);
			for (auto& c : pinOutSelection.connections) {
				c.RecacheConverts();
			}

			ResizeGatedValues();
		};
	}

	gatesCount += 1;
	PinInOptions options;
	PinInput pinIn = SetupInputPin(inputPinType, this, nullptr, 1, name);
	pinIn.SetEnabled(pinInputs.size() - 1 == selectedGate);

	pinIn.SetOnConnected(std::move(connectCb));
	pinInputs.push_back(std::move(pinIn));

	ResizeGatedValues();
	return &pinInputs.back();
}