#include "channelMap.h"
#include "imgui_stdlib.h"

using namespace seam;
using namespace seam::nodes;

const size_t ChannelMap::maxChannels = 16;

ChannelMap::ChannelMap() : IDynamicPinsNode("Channel Map") {
   ResizeInputBuffer();
}

ChannelMap::~ChannelMap() {

}

PinInput* ChannelMap::PinInputs(size_t& size) {
    size = pinInputs.size();
    return &pinInputs[0];
}

PinOutput* ChannelMap::PinOutputs(size_t& size) {
    if (currentInputType != PinType::Any) {
        size = pinOutputs.size();
        return &pinOutputs[0];
    }
    size = 0;
	return nullptr;
}

void ChannelMap::Update(UpdateParams* params) {
    const size_t bytesPerInput = BytesPerInput();
    for (size_t i = 0; i < pinOutputs.size(); i++) {
        // Use the PinOutMapping to determine where the input buffer data is.
        auto& outMap = pinOutMappings[i];
        std::array<char, 16 * sizeof(float)> outBuff = { 0 };

        // Copy each element from the input buffer to a temp output buffer that contains all channel values in sequence.
        for (size_t chan = 0; chan < outMap.channelsCount; chan++) {
            uint16_t assignedInput = outMap.assignedInputs[chan];
            uint16_t assignedChannel = outMap.assignedChannels[chan];

            size_t inputOffset = assignedInput * bytesPerInput + assignedChannel * pinElementSize;
            std::copy(&inputBuffer[inputOffset], &inputBuffer[inputOffset + pinElementSize], &outBuff[chan * pinElementSize]);
        }

        // Finally, push using the temp buffer.
        params->push_patterns->Push(pinOutputs[i], outBuff.data(), outMap.channelsCount * pinElementSize);
    }

}

/*
void ChannelMap::OnPinConnected(PinConnectedArgs args) {
    // If an input pin was connected
    if (FindPinInput(args.pinIn->id)) {
        if (currentInputType == PinType::Any) {
            // Set our type to the Output's type.
            SetInputType(args.pinOut->type);
        }

        // If all inputs are now connected, add another input of the current type.
        bool allConnected = true;
        for (size_t i = 0; i < pinInputs.size(); i++) {
            allConnected = allConnected && pinInputs[i].connection != nullptr;
        }

        if (allConnected) {
            CreateInput();
        }
    }
    else {
        // An output was connected, nothing to do?
    }
}
*/

bool ChannelMap::GuiDrawPropertiesList(UpdateParams* params) {
    bool changed = false;

    if (ImGui::TreeNode("Inputs")) {
        for (size_t i = 0; i < pinInputs.size(); i++) {
            ImGui::PushID(i);
            std::string inputName = "Input " + std::to_string(i);
            ImGui::InputText(inputName.c_str(), &pinInputs[i].name);

            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                DeleteInput(i);
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Outputs")) {
        // Don't allow outputs to be added until the input type is known.
        if (currentInputType != PinType::Any && ImGui::Button("Add Output")) {
            CreateOutput();
        }
        
        // Draw configuration for each pin out.
        for (size_t i = 0; i < pinOutputs.size(); i++) {
            ImGui::PushID(i);
            auto& pinMap = pinOutMappings[i];
            std::string outputName = "Output " + std::to_string(i);

            ImGui::InputText(outputName.c_str(), &pinOutputs[i].name);
            ImGui::SameLine();
            if (ImGui::Button("Delete")) {
                DeleteOutput(i);
                continue;
            }

            int input = pinMap.channelsCount;
            ImGui::DragInt("Channels", &input, 1.0f, 1, 16);
            pinMap.channelsCount = (uint16_t)input;

            // Draw per-channel configuration.
            for (uint16_t j = 0; j < pinMap.channelsCount; j++) {
                ImGui::PushID(i * 16 + j);

                // Draw the combo box that lets you select the mapped input pin by name.
                uint16_t assignedPinIndex = pinMap.assignedInputs[j];
                PinInput& assignedPinIn = pinInputs[assignedPinIndex];
                if (ImGui::BeginCombo("Mapped Input", assignedPinIn.name.c_str())) {
                    for (size_t pi = 0; pi < pinInputs.size(); pi++) {
                        const bool isSelected = assignedPinIndex == pi;
                        if (ImGui::Selectable(pinInputs[pi].name.c_str(), isSelected)) {
                            pinMap.assignedInputs[j] = pi;
                        }

                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                input = pinMap.assignedChannels[j];
                ImGui::DragInt("In Channel", &input, 1.0f, 0, 16);
                pinMap.assignedChannels[j] = (uint16_t)input;

                ImGui::PopID();
            }

            ImGui::PopID();
        }

        ImGui::TreePop();
    }

    return changed;
}

std::vector<props::NodeProperty> ChannelMap::GetProperties() {
    // Order matters here: Inputs and Outputs count should be set before other props,
    // so add those to the vector first.
    auto props = std::vector<props::NodeProperty>();

    uint32_t inputsCount = pinInputs.size();
    props.push_back(props::SetupUintProperty("Inputs Count", [this, &inputsCount](size_t& size) {
        size = 1;
        return &inputsCount;
    }, [this, &inputsCount](uint32_t* newInputsSize, size_t shouldBe1) {
        assert(shouldBe1 == 1);
        while (inputsCount < *newInputsSize) {
            CreateInput();
        }
        while (inputsCount > *newInputsSize) {
            DeleteInput(inputsCount - 1);
        }

        assert(inputsCount == pinInputs.size() && inputsCount == *newInputsSize);
    }));

    uint32_t outputsCount = pinOutputs.size();
    props.push_back(props::SetupUintProperty("Outputs Count", [this, &outputsCount](size_t& size) {
        size = 1;
        return &outputsCount;
    }, [this, &outputsCount](uint32_t* newOutputsSize, size_t shouldBe1) {
        assert(shouldBe1 == 1);
        while (outputsCount < *newOutputsSize) {
            CreateOutput();
        }
        while (outputsCount > *newOutputsSize) {
            DeleteOutput(outputsCount - 1);
        }

        assert(outputsCount == pinOutputs.size() && outputsCount == *newOutputsSize);
    }));


    return props;

}

void ChannelMap::SetInputType(PinType type) {
    currentInputType = pinInputs[0].type = type;
    if (!pinOutputs.size()) {
        // Create an initial Output using the same type as the just-connected type.
        CreateOutput();
    }

    if (currentInputType != PinType::Any) {
        for (auto& conn : pinOutputs[0].connections) {
            conn.RecacheConverts();
        }
        pinElementSize = pins::PinTypeToElementSize(currentInputType);
    }
}

void ChannelMap::ResizeInputBuffer() {
    const auto bytesPerInput = BytesPerInput();
    inputBuffer.resize(bytesPerInput * pinInputs.size());
    for (size_t i = 0; i < pinInputs.size(); i++) {
        pinInputs[i].SetBuffer(&inputBuffer[i * bytesPerInput], maxChannels);
    }
}

inline size_t ChannelMap::BytesPerInput() {
    return pinElementSize * maxChannels;
}

PinInput& ChannelMap::CreateInput() {
    pinInputs.push_back(pins::SetupInputPin(currentInputType, this, &inputBuffer[0], 
        16, "Channels " + std::to_string(pinInputs.size())));
    ResizeInputBuffer();
    RecacheInputConnections();
    return pinInputs[pinInputs.size() - 1];
}

PinOutput& ChannelMap::CreateOutput() {
    std::string outputName = "Output " + std::to_string(pinOutputs.size());
    pinOutputs.push_back(pins::SetupOutputPin(this, currentInputType, outputName));
    pinOutMappings.push_back(PinOutMapping());
    return pinOutputs[pinOutputs.size() - 1];
}

void ChannelMap::DeleteInput(size_t i) {
    size_t bytesPerInput = BytesPerInput();

    // Copy later input buffer data forward.
    std::copy(inputBuffer.begin() + bytesPerInput * (i + 1), inputBuffer.end(), 
        inputBuffer.begin() + bytesPerInput * i);

    if (pinInputs[i].connection != nullptr) {
        // PLS FIX ME... Need to be able to disconnect without the Editor.
        // Refactor the Editor so it doesn't manage connections itself,
        // and then you can use the connector class from here.
        assert(false);
    }

    pinInputs.erase(pinInputs.begin() + i);
    ResizeInputBuffer();
}

void ChannelMap::DeleteOutput(size_t i) {
    pinOutputs.erase(pinOutputs.begin() + i);
    pinOutMappings.erase(pinOutMappings.begin() + i);
}