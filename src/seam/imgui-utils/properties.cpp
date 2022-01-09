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

	bool Draw(std::string_view label, std::string& v) {
		char buf[256] = { 0 };
		// TODO not strcpy
		strncpy(buf, v.c_str(), std::min(v.size(), 256ULL));

		if (ImGui::InputText(label.data(), buf, 256)) {
			v = buf;
			return true;
		}
		return false;
	}

	bool DrawShaderPath(std::string_view label, std::string& path) {
		// refresh button needs a logo
		bool refresh_requested = ImGui::Button("refresh shader");
		bool name_changed = Draw(label, path);
		return refresh_requested || name_changed;
	}

	bool DrawPin(std::string_view name, PinFloatMeta* pin, float* channels, const size_t size, float speed) {
		bool changed = false;
		std::string chan_name;
		for (size_t i = 0; i < size; i++) {
			// this should be smarter eventually
			// like, draw 1-4 channels in the same line
			chan_name = std::string(name) + "[" + std::to_string(i) + "]";
			changed = Draw(chan_name, channels[i], pin->min, pin->max, speed) || changed;
		}
		return changed;
	}

	bool DrawPin(std::string_view name, PinIntMeta* pin, int* channels, const size_t size, float speed) {
		bool changed = false;
		std::string chan_name;
		for (size_t i = 0; i < size; i++) {
			// same as above, this could be smarter
			chan_name = std::string(name) + "[" + std::to_string(i) + "]";
			changed = Draw(chan_name, channels[i], pin->min, pin->max, speed) || changed;
		}
		return changed;
	}

	bool DrawPinInput(IPinInput* input) {

		size_t num_channels = 0;
		void* channels = input->GetChannels(num_channels);
		switch (input->type) {
		case PinType::INT: {
			PinIntMeta* meta = dynamic_cast<PinIntMeta*>(input);
			return DrawPin(input->name, meta, (int*)channels, num_channels);
		}
		case PinType::FLOAT: {
			PinFloatMeta* meta = dynamic_cast<PinFloatMeta*>(input);
			return DrawPin(input->name, meta, (float*)channels, num_channels);
		}
		default:
			// TODO
			assert(false);
			return false;
		}
	}

	bool DrawPinInputs(nodes::INode* node) {
		bool changed = false;
		size_t size;
		IPinInput** inputs = node->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			changed = DrawPinInput(inputs[i]) || changed;
		}
		return changed;
	}

}