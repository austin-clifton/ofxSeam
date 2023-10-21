#pragma once

#include "iNode.h"

using namespace seam::pins;

namespace seam::nodes {
	class Gate : public IDynamicPinsNode {
	public:
		Gate();
		~Gate();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		PinInput* AddPinIn(PinInArgs args) override;
		
		PinOutput* AddPinOut(PinOutput&& pinOut, size_t index) override;

		bool GuiDrawPropertiesList() override;

	private:
		PinInput* AddGatePin(const std::string_view name);
		
		uint32_t gatesCount = 0;
		int selectedGate = 0;

		std::vector<PinInput> pinInputs;
		std::vector<float> gatedValues;

		PinOutput pinOutSelection = pins::SetupOutputPin(this, pins::PinType::FLOAT, "Gated Value");
		PinOutput pinOutGateChangedEvent = pins::SetupOutputPin(this, pins::PinType::FLOW, "Gate Changed Event");
	};
}