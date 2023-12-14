#include "threshold.h"

using namespace seam::nodes;

Threshold::Threshold() : INode("Threshold") {
    flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);

    VectorPinInput::Options options;
    options.onSizeChanged = [this](VectorPinInput* vectorPin) { OnSizeChanged(vectorPin); };

    VectorPinInput::Options options2;
    options2.onSizeChanged = [this](VectorPinInput* vectorPin) { OnSizeChanged(vectorPin); };

    thresholds.SetCallbackOptions(std::move(options));
    values.SetCallbackOptions(std::move(options2));
};

void Threshold::Update(UpdateParams* params) {
    size_t thresholdsSize, valuesSize;
    ThresholdConfig* configs = thresholds.Get<ThresholdConfig>(thresholdsSize);
    float* fv = values.Get<float>(valuesSize);

    assert(thresholdsSize == valuesSize);

    for (size_t i = 0; i < valuesSize; i++) {
        size_t childrenSize;
        PinOutput* childPins = pinOutEvents[i].PinOutputs(childrenSize);
        PinOutput& flowPinOut = childPins[0];
        PinOutput& triggeredPinOut = childPins[1];

        if (configs[i].triggered) {
            // When triggered and the current value has dipped below the trigger, accumulate silence time.
            if (configs[i].triggerValue > fv[i]) {
                configs[i].timePastThreshold += params->delta_time;
                // If this threshold has been quiet long enough, un-trigger it.
                if (configs[i].timePastThreshold > configs[i].silenceTime) {
                    configs[i].triggered = false;
                    configs[i].timePastThreshold = 0.f;

                    bool pushValues[] = { false };
                    params->push_patterns->Push(triggeredPinOut, pushValues, 1);
                }
            } else {
                // Went back into triggered state, reset time past threshold.
                configs[i].timePastThreshold = 0.f;
            }
        } else {
            // When not triggered and the current value exceeds the trigger, accumulate sustain time.
            if (configs[i].triggerValue <= fv[i]) {
                configs[i].timePastThreshold += params->delta_time;

                // If this threshold has been exceeded for long enough, trigger it.
                if (configs[i].timePastThreshold > configs[i].sustainTime) {
                    configs[i].triggered = true;
                    configs[i].timePastThreshold = 0.f;

                    bool pushValues[] = { true };
                    params->push_patterns->Push(triggeredPinOut, pushValues, 1);
                    params->push_patterns->PushFlow(flowPinOut);
                }
            } else {
                // Dipped below the trigger threshold, reset sustain time.
                configs[i].timePastThreshold = 0.f;
            }
        }
    }
}

PinInput* Threshold::PinInputs(size_t& size) {
    size = 2;
    return &pinInThresholds;
}

PinOutput* Threshold::PinOutputs(size_t& size) {
    size = pinOutEvents.size();
    return pinOutEvents.data();
}

PinInput Threshold::CreateThresholdPin(VectorPinInput* vectorPin, size_t i) {
    PinInput pinIn = SetupInputPin(PinType::STRUCT, this, nullptr, 0, "Threshold " + std::to_string(i));
    // Set up a child pin per member in ThresholdConfig
    std::vector<PinInput> children = {
        SetupInputPin(PinType::FLOAT, this, nullptr, 0, "Threshold"),
        SetupInputPin(PinType::FLOAT, this, nullptr, 0, "Sustain Time"),
        SetupInputPin(PinType::FLOAT, this, nullptr, 0, "Silence Time"),
    };

    pinIn.SetChildren(std::move(children));
    return pinIn;
}

void Threshold::SetThresholdPinBuffers(void* ptr, PinInput* thresholdPin) {
    ThresholdConfig* config = (ThresholdConfig*)ptr;
    // Reset to initial struct values.
    *config = ThresholdConfig();
    size_t size;
    PinInput* children = thresholdPin->PinInputs(size);
    assert(size == 3);

    children[0].SetBuffer(&config->triggerValue, 1);
    children[1].SetBuffer(&config->sustainTime, 1);
    children[2].SetBuffer(&config->silenceTime, 1);
}

void Threshold::OnSizeChanged(VectorPinInput* changed) {
    const size_t newSize = changed->Size();

    thresholds.UpdateSize(newSize);
    values.UpdateSize(newSize);

    const size_t currSize = pinOutEvents.size();
    pinOutEvents.resize(newSize);
    for (size_t i = currSize; i < newSize; i++) {
        // Each output pin gets a struct containing a "triggered" flow event pin,
        // and a bool pin that is enabled while triggered.
        pinOutEvents[i] = SetupOutputPin(this, PinType::STRUCT, "Threshold " + std::to_string(i));
        std::vector<PinOutput> children = {
            SetupOutputPin(this, PinType::FLOW, "Triggered Event"),
            SetupOutputPin(this, PinType::BOOL, "Triggered State"),
        };

        pinOutEvents[i].SetChildren(std::move(children));
    }
}

void Threshold::GuiDrawNodeCenter() {
    size_t size;
    ThresholdConfig* configs = thresholds.Get<ThresholdConfig>(size);

    for (size_t i = 0; i < size; i++) {
        if (i > 0 && i % 4 == 0) {
            ImGui::NewLine();
        } else {
            ImGui::SameLine();
        }

        ImVec4 col = configs[i].triggered ? ImVec4(1.0, 0.0, 1.0, 1.0) : ImVec4(0.1f, 0.1f, 0.1f, 1.f);
        ImGui::TextColored(col, "o");
    }
}

bool Threshold::GuiDrawPropertiesList(UpdateParams* params) {
    ImGui::Text("Edit All Thresholds:");
    bool triggerChanged = ImGui::DragFloat("Trigger Value", &defaultConfig.triggerValue);
    bool sustainChanged = ImGui::DragFloat("Sustain Time", &defaultConfig.sustainTime);
    bool silenceChanged = ImGui::DragFloat("Silence Time", &defaultConfig.silenceTime);
    bool changed = triggerChanged || sustainChanged || silenceChanged;

    if (changed) {
        size_t thresholdsSize;
        ThresholdConfig* configs = thresholds.Get<ThresholdConfig>(thresholdsSize);

        if (triggerChanged) {
            for (size_t i = 0; i < thresholdsSize; i++) {
                configs[i].triggerValue = defaultConfig.triggerValue;
            }
        }

        if (sustainChanged) {
            for (size_t i = 0; i < thresholdsSize; i++) {
                configs[i].sustainTime = defaultConfig.sustainTime;
            }
        }

        if (silenceChanged) {
            for (size_t i = 0; i < thresholdsSize; i++) {
                configs[i].silenceTime = defaultConfig.silenceTime;
            }
        }
    }

    ImGui::NewLine();
    return changed;
}