#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	class Gate : public INode {
	public:
		Gate();
		~Gate();

		void Setup(SetupParams* params) override;

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		std::vector<props::NodeProperty> GetProperties() override;

	private:
		PinInput* AddGatePin(const std::string_view name);
		void UpdateGatesCount(uint32_t newCount);
		void UpdateNumCoords(uint16_t newNumCoords);
		void ResizeGatedValues();
		
		uint32_t gatesCount = 0;
		uint32_t selectedGate = 0;
		uint32_t numCoords = 1;

		std::vector<PinInput> pinInputs;
		std::vector<float> gatedValues;

		PinOutput pinOutSelection = pins::SetupOutputPin(this, pins::PinType::FLOAT, "Gated Value");
		PinOutput pinOutGateChangedEvent = pins::SetupOutputPin(this, pins::PinType::FLOW, "Gate Changed Event");
	};
}