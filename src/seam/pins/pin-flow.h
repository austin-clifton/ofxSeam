#pragma once

#include "pin-input.h"

namespace seam::pins {
	// flow pins are always a single channel since they hold no state,
	// and are only used to dirty nodes
	struct PinFlow : public PinInput<bool, 0> {
		PinFlow() : PinInput<bool, 0>() {
			type = PinType::FLOW;
		}

		// flow pins have no value, the node just needs to be dirtied and updated

	};
}
