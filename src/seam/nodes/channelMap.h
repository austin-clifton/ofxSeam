#pragma once

#include "iNode.h"

#include "seam/pins/pin.h"
#include "seam/properties/nodeProperty.h"

using namespace seam::pins;

namespace seam::nodes {
    /// @brief Provides tighter control over output-to-input channel mappings.
	class ChannelMap : public IDynamicPinsNode {
	public:
		ChannelMap();
		~ChannelMap();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

        void OnPinConnected(PinConnectedArgs args) override;

		void OnPinDisconnected(pins::PinInput* pinIn, pins::PinOutput* pinOut) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

        std::vector<props::NodeProperty> GetProperties() override;

        //pins::PinInput* AddPinIn(PinInArgs args) override;
		
        //pins::PinOutput* AddPinOut(pins::PinOutput&& pinOut, size_t size) override;

	private:
        static const size_t maxChannels;

        struct PinOutMapping {
            uint16_t channelsCount = 1;
            std::array<uint16_t, 16> assignedInputs = { 0 };
            std::array<uint16_t, 16> assignedChannels = { 0 };
        };

        inline size_t BytesPerInput();
        void SetInputType(PinType type);
        void ResizeInputBuffer();

        PinInput& CreateInput();
        void DeleteInput(size_t i);
        PinOutput& CreateOutput();
        void DeleteOutput(size_t i);

        PinType currentInputType = PinType::ANY;
        size_t pinElementSize = 0;
        std::vector<char> inputBuffer;
        size_t nextAvailableByte = 0;

        std::vector<PinInput> pinInputs = {
            pins::SetupInputPin(PinType::ANY, this, &inputBuffer[0], 16, "Channels In"),
        };

        std::string outputName = "Output XX";

        std::vector<PinOutput> pinOutputs;
        std::vector<PinOutMapping> pinOutMappings;
	};
}