#pragma once

#include "../pins/pin.h"
#include "../pins/pin-float.h"
#include "../pins/pin-int.h"
#include "../pins/pin-uint.h"
#include "../nodes/i-node.h"

namespace seam {
	namespace props {
		/// Draw an add button with an input box and label.
		/// \param label The name of the button
		/// \param out_val Will be set to the user input value 
		/// when the button is pressed
		/// \return true if the button was pressed
		template <typename T>
		bool DrawAddButton(
			std::string_view label, 
			T& out_val,
			T min, 
			T max, 
			float speed = 0.01f, 
			ImGuiSliderFlags flags = 0
		) {
			// draw the + button
			bool pressed = ImGui::Button("+");
			ImGui::SameLine();
			// draw the label and input box
			// note we don't care if the value changed; 
			// the value only matters when the button was pressed
			Draw(label, out_val, min, max, speed, flags);
			return pressed;
		}

		bool Draw(std::string_view label, std::string& v);

		/// draws a reload button in addition to providing a path setter
		bool DrawShaderPath(std::string_view label, std::string& path);

		/// generic for drawing different input pin types
		bool DrawPinInput(pins::IPinInput* input);

		/// utility function for drawing all a node's input pins
		bool DrawPinInputs(nodes::INode* node);

		// this probably doesn't belong here long-term
		void DrawFbo(const ofFbo& fbo);

		// ...TODO
	}
}