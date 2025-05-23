#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// @brief Selects one input from a user-defined list of inputs using a select line.
	class Select : public IDynamicPinsNode {
	public:
		Select();
		~Select();

		void Setup(SetupParams* params) override;

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		std::vector<props::NodeProperty> GetProperties() override;

		pins::PinInput* AddPinIn(PinInArgs args) override;

	private:
		PinInput* AddGatePin(const std::string_view name);
		void UpdateGatesCount(uint32_t newCount);
		void UpdateNumCoords(uint16_t newNumCoords);
		void ResizeGatedValues();
		
		uint32_t gatesCount = 0;
		uint32_t lastSelectedGate = 0;
		uint32_t selectedGate = 0;
		uint32_t numCoords = 1;
		PinType inputPinType = PinType::Any;

		std::vector<PinInput> pinInputs;
		std::vector<std::byte> gatedValues;

		PinOutput pinOutSelection = pins::SetupOutputPin(this, pins::PinType::Any, "Selected Value");
		PinOutput pinOutGateChangedEvent = pins::SetupOutputPin(this, pins::PinType::Flow, "On Selection Changed");
	};
}