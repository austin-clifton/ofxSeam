#include "pin.h"

namespace seam {
	PinInput SetupPinInput(Pin* pin, IEventNode* node) {
		assert(pin && node);
		PinInput input;
		input.pin = pin;
		input.node = node;
		return input;
	}

	Pin SetupPinOutput(PinType type, std::string_view name, PinFlags flags) {
		Pin pin;
		pin.type = type;
		pin.name = name;
		pin.flags = (PinFlags)(flags | PinFlags::OUTPUT);
		return pin;
	}
}