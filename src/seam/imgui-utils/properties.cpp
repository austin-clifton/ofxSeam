#include <string>

#include "ofxImGui.h"

#include "properties.h"

using namespace seam::pins;

namespace seam::props {
	bool DrawVectorResize(PinInput* pinIn) {
		VectorPinInput* v = (VectorPinInput*)pinIn->seamp;
		std::string name = pinIn->name + " Size";
		size_t size = v->Size();
		if (ImGui::DragScalar(name.c_str(), ImGuiDataType_U64, &size)) {
			// Resize requested.
			v->UpdateSize(size);
			return true;
		}
		return false;
	}

	bool DrawTextInput(std::string_view label, std::string& v) {
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
		bool name_changed = DrawTextInput(label, path);
		return refresh_requested || name_changed;
	}

	bool DrawPinInput(PinInput* input) {
		bool sizeChanged = false;

		// If this is a vector pin, draw the vector resize GUI.
		if ((input->flags & PinFlags::VECTOR) > 0) {
			sizeChanged = DrawVectorResize(input);
		}

		bool pinsChanged = false;

		size_t num_channels = 0;
		void* channels = input->GetChannels(num_channels);
		switch (input->type) {
		case PinType::INT: {
			PinIntMeta* meta = (PinIntMeta*)input->PinMetadata();
			pinsChanged = ImGui::DragScalarN(
				input->name.c_str(),
				ImGuiDataType_S32,
				channels,
				num_channels,
				0.01f,
				meta ? &meta->min : nullptr,
				meta ? &meta->max : nullptr,
				"%d",
				0
			);
			break;
		}
		case PinType::UINT: {
			PinUintMeta* meta = (PinUintMeta*)input->PinMetadata();
			pinsChanged = ImGui::DragScalarN(
				input->name.c_str(),
				ImGuiDataType_U32,
				channels,
				num_channels,
				0.05f,
				meta ? &meta->min : nullptr,
				meta ? &meta->max : nullptr,
				"%u",
				0
			);
			break;
		}
		case PinType::FLOAT: {
			PinFloatMeta* meta = (PinFloatMeta*)input->PinMetadata();
			pinsChanged = ImGui::DragScalarN(
				input->name.c_str(),
				ImGuiDataType_Float,
				channels,
				num_channels,
				0.001f,
				meta ? &meta->min : nullptr,
				meta ? &meta->max : nullptr,
				"%.3f",
				0
			);
			break;
		}
		case PinType::BOOL: {
			// TODO how to draw more than one bool?
			assert(num_channels == 1);
			pinsChanged = ImGui::Checkbox(input->name.c_str(), (bool*)channels);
			break;
		}
		case PinType::FLOW: {
			if (ImGui::Button(input->name.c_str())) {
				input->Callback();
				pinsChanged = true;
			}
			break;
		}
		case PinType::ANY:
		case PinType::NOTE_EVENT:
		case PinType::FBO:
		{
			// TODO maybe allow note events to be "mocked" here;
			// there isn't really anything else to draw for these otherwise.
			break;
		}
		case PinType::STRUCT:
		{
			// Draw child pins; should this just be done for all pin inputs though...?
			size_t childrenSize; 
			PinInput* children = input->PinInputs(childrenSize);
			ImGui::Indent();
			ImGui::Text("%s", input->name.c_str());
			for (size_t i = 0; i < childrenSize; i++) {
				pinsChanged = DrawPinInput(&children[i]) || pinsChanged;
			}
			ImGui::Unindent();
			break;
		}
		default:
			throw std::logic_error("DrawPinInputs(): Not implemented (please implement me)");
		}

		return pinsChanged || sizeChanged;
	}

	bool DrawPinInputs(nodes::INode* node) {
		bool changed = false;
		size_t size;
		PinInput* inputs = node->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			changed = DrawPinInput(&inputs[i]) || changed;
		}
		return changed;
	}

	void DrawFbo(const ofFbo& fbo) {
		ImVec2 wsize = ImGui::GetContentRegionAvail();
		// oof
		// https://forum.openframeworks.cc/t/how-to-draw-an-offbo-in-ofximgui-window/33174/2
		ImTextureID texture_id = (ImTextureID)(uintptr_t)fbo.getTexture().getTextureData().textureID;
		ImGui::Image(texture_id, ImVec2(256, 256));
	}
}