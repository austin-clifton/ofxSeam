#pragma once

#include "pin.h"

namespace seam::pins {
	// flow pins are always a single channel since they hold no state,
	// and are only used to fire an some event handler on the receiving node
	struct PinFlow : public PinInput<bool, 0> {
		std::function<void(void)> Callback;

		PinFlow(
			std::function<void(void)>&& _Callback,
			std::string_view _name = "flow",
			std::string_view _description = "",
			PinFlags _flags = PinFlags::INPUT
		) 
			: PinInput<bool, 0>(), Callback(std::move(_Callback))
		{
			name = _name;
			description = _description;
			flags = (PinFlags)(flags | _flags);
			type = PinType::FLOW;
		}
	};
}
