#include "seam/nodes/toggle.h"

using namespace seam::nodes;
using namespace seam::pins;

Toggle::Toggle() : INode("Toggle") {
}

void Toggle::Setup(SetupParams* params) {
    ReconfigureInputs(2);
}

PinInput* Toggle::PinInputs(size_t& size) {
    size = pinInputs.size();
    return pinInputs.data();
}

PinOutput* Toggle::PinOutputs(size_t& size) {
    size = 1;
    return &pinOutSelection;
}

void Toggle::Update(UpdateParams* params) {
    params->push_patterns->PushSingle(pinOutSelection, &currentState);
}

bool Toggle::GuiDrawPropertiesList(UpdateParams* params) {
    uint32_t newCount = statesCount;
    if (ImGui::DragScalar("States Count", ImGuiDataType_U32, &newCount)) {
        ReconfigureInputs(newCount);
        return true;
    }   
    return false;
}

std::vector<seam::props::NodeProperty> Toggle::GetProperties() {
	using namespace seam::props; 

    std::vector<NodeProperty> props;
    props.push_back(SetupUintProperty(
        "States Count", [this](size_t& size) {
        size = 1;
        return &statesCount;
    }, [this](uint32_t* newCount, size_t size) {
        assert(size == 1);
        ReconfigureInputs(*newCount);
    }));

    return props;
}

void Toggle::ReconfigureInputs(uint32_t newCount) {
    statesCount = std::max((uint32_t)2, newCount);

    // Add new state triggers.
    for (size_t i = pinInputs.size(); i < statesCount; i++) {
        pinInputs.push_back(SetupInputFlowPin(this, 
            [this, i] {
                currentState = i;
            }, 
            "Toggle " + std::to_string(i))
        );
    }

    // Remove any state triggers that should no longer exist.
    if (statesCount < pinInputs.size()) {
        pinInputs.resize(statesCount);
    }

    RecacheInputConnections();
}