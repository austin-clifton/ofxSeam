#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	/// @brief Toggle between two or more states.
	/// For now output just ranges from [0..statesCount], but ideally should allow user-defined mappings as well.
	class Toggle : public INode {
	public:
		Toggle();

		void Setup(SetupParams* params) override;
        
		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

        void Update(UpdateParams* params) override;

		std::vector<props::NodeProperty> GetProperties() override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

	private:
		/// @brief To be called whenever statesCount should be updated.
		void ReconfigureInputs(uint32_t newStatesCount);

		uint32_t statesCount = 2;
		uint32_t currentState = 0;

		std::vector<PinInput> pinInputs;

		PinOutput pinOutSelection = SetupOutputPin(this, PinType::Uint, "Selection");
	};
}
