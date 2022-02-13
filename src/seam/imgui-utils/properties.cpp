#include <string>

#include "imgui/src/imgui.h"

#include "properties.h"

using namespace seam::pins;

namespace seam::props {
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

	bool DrawPinInput(IPinInput* input) {
		size_t num_channels = 0;
		void* channels = input->GetChannels(num_channels);
		switch (input->type) {
		case PinType::INT: {
			PinIntMeta* meta = dynamic_cast<PinIntMeta*>(input);
			return ImGui::DragScalarN(
				input->name.c_str(),
				ImGuiDataType_S32,
				channels,
				num_channels,
				0.01f,
				&meta->min,
				&meta->max,
				"%d",
				0
			);
		}
		case PinType::FLOAT: {
			PinFloatMeta* meta = dynamic_cast<PinFloatMeta*>(input);
			return ImGui::DragScalarN(
				input->name.c_str(),
				ImGuiDataType_Float,
				channels,
				num_channels,
				0.001f,
				&meta->min,
				&meta->max,
				"%.3f",
				0
			);
		}
		case PinType::NOTE_EVENT: {
			// TODO maybe allow note events to be "mocked" here;
			// there isn't really anything else to draw for these otherwise.
			return false;
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