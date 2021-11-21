#include "pin.h"

namespace seam {
	template <>
	PinInput CreatePinInput<int>(const std::string_view name, std::function<void(int)>&& callback) {
		PinInput input;
		PinInt* pin = new PinInt();
		pin->name = name;
		pin->callback = std::move(callback);
		input.pin = pin;
		return input;
	}

	template <>
	PinInput CreatePinInput<float>(const std::string_view name, std::function<void(float)>&& callback) {
		PinInput input;
		PinFloat* pin = new PinFloat();
		pin->name = name;
		pin->callback = std::move(callback);
		input.pin = pin;
		return input;
	}

	template<>
	PinInput CreatePinInput<Texture>(const std::string_view name, std::function<void(Texture)>&& callback) {
		PinInput input;
		PinTexture* pin = new PinTexture();
		pin->name = name;
		pin->callback = std::move(callback);
		input.pin = pin;
		return input;
	}

	// TODO there are more of these...
}