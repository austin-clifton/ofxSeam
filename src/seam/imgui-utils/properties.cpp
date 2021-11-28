#include <string>

#include "imgui/src/imgui.h"

#include "properties.h"

namespace seam::props {

	bool Draw(
		std::string_view label,
		float& v,
		float min,
		float max,
		float speed,
		ImGuiSliderFlags flags
	) {
		return ImGui::DragFloat(label.data(), &v, speed, min, max, "%.2f", flags);
	}

	bool Draw(
		std::string_view label,
		int& v,
		int min,
		int max,
		float speed,
		ImGuiSliderFlags flags
	) {
		return ImGui::DragInt(label.data(), &v, speed, min, max, "%d", flags);
	}

	bool Draw(std::string_view label, std::string& v, bool is_shader_path) {
		char buf[256] = { 0 };
		// TODO not strcpy
		strncpy(buf, v.c_str(), std::min(v.size(), 256ULL));

		// refresh button needs a logo
		bool refresh_requested = ImGui::Button("refresh shader");

		if (ImGui::InputText(label.data(), buf, 256)) {
			v = buf;
			return true;
		}
		return refresh_requested;
	}

	bool DrawPin(PinFloat* pin, float speed) {
		return Draw(pin->name.data(), pin->value, pin->min, pin->max, speed);
	}

	bool DrawPin(PinInt* pin, float speed) {
		return Draw(pin->name.data(), pin->value, pin->min, pin->max, speed);
	}

	bool DrawPinInput(PinInput input) {
		// TODO should connected pins be settable?

		switch (input.pin->type) {
		case PinType::INT:
			return DrawPin((PinInt*)input.pin);
		case PinType::FLOAT:
			return DrawPin((PinFloat*)input.pin);
		default:
			// TODO
			assert(false);
			return false;
		}
	}

	bool DrawPinInputs(IEventNode* node) {
		bool changed = false;
		size_t size;
		PinInput* inputs = node->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			changed = DrawPinInput(inputs[i]) || changed;
		}
		return changed;
	}

}