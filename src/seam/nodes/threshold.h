#pragma once

#include "seam/nodes/iNode.h"

using namespace seam::pins;

namespace seam::nodes {
    /// @brief Fires events when an input line exceeds some threshold for an amount of time.
    class Threshold : public INode {
    public:
        Threshold();

        void Update(UpdateParams* params) override;

        PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

        // bool GuiDrawPropertiesList(UpdateParams* params) override;

    private:
        struct ThresholdConfig {
            /// @brief When the value crosses this threshold for longer than sustainTime,
            /// an event is fired and the threshold enters a triggered state.
            float triggerValue = 0.8;

            /// @brief An event won't be fired unless the triggerValue is crossed for this long.
            float sustainTime = 0.1;

            /// @brief After an event is fired, another threshold event won't be fired 
            /// until the value crosses back to the untriggered side of the triggerValue for this long.
            float silenceTime = 0.5;

            // These two are internal to the Node, and aren't pin-mapped.
            float timePastThreshold = 0.f;            
            bool triggered = false;
        };

        PinInput CreateThresholdPin(VectorPinInput* vectorPin, size_t i);
        void SetThresholdPinBuffers(void* ptr, PinInput* thresholdPin);
        void OnSizeChanged(VectorPinInput* vectorPin);

        VectorPinInput thresholds = VectorPinInput(
            std::bind(&Threshold::CreateThresholdPin, 
                this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&Threshold::SetThresholdPinBuffers, 
                this, std::placeholders::_1, std::placeholders::_2),
            sizeof(ThresholdConfig)
        );

        // TODO use the GUI to set up the default threshold config since it's fucking annoying to edit.
        // ThresholdConfig defaultConfig;

        VectorPinInput values = VectorPinInput(PinType::FLOAT);
        
        PinInput pinInThresholds = thresholds.SetupVectorPin(this, PinType::STRUCT, "Thresholds");
        PinInput pinInValues = values.SetupVectorPin(this, PinType::FLOAT, "Values");

        std::vector<PinOutput> pinOutEvents;
    };
}