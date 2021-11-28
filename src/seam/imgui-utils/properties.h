#pragma once

#include "../pin.h"
#include "../event-nodes/event-node.h"

namespace seam {
	namespace props {
		/// these are for drawing properties which are not pins
		bool Draw(
			std::string_view label,
			float& v,
			float min = -FLT_MAX,
			float max = FLT_MAX,
			float speed = 1.f,
			ImGuiSliderFlags flags = 0
		);
		bool Draw(
			std::string_view label,
			int& v,
			int min = INT_MIN,
			int max = INT_MAX,
			float speed = 1.f,
			ImGuiSliderFlags flags = 0
		);

		bool Draw(std::string_view label, std::string& v, bool is_shader_path = false);

		bool DrawPin(PinFloat* pin, float speed = 1.f);
		bool DrawPin(PinInt* pin, float speed = .2f);

		/// generic for drawing different input pin types
		bool DrawPinInput(PinInput input);

		/// utility function for drawing all a node's input pins
		bool DrawPinInputs(IEventNode* node);

		// ...TODO
	}
}