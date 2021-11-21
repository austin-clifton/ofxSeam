#include "event-node.h"
#include "imgui/src/imgui.h"
#include "imgui/src/blueprints/widgets.h"

using namespace seam;
namespace im = ImGui;
namespace ed = ax::NodeEditor;

// TODO make this configurable
float icon_size = 24.f;

namespace {
	// next two are stripped from the blueprints example in imgui-node-editor
	// TODO make the pin icons more meaningful

	ImColor GetIconColor(PinType type)
	{
		switch (type) {
		default:
		case PinType::NONE:
		case PinType::FLOW:     return ImColor(255, 255, 255);
		case PinType::BOOL:     return ImColor(220, 48, 48);
		case PinType::INT:      return ImColor(68, 201, 156);
		case PinType::FLOAT:    return ImColor(147, 226, 74);
		case PinType::STRING:   return ImColor(124, 21, 153);
		case PinType::TEXTURE:  return ImColor(51, 150, 215);
		// case PinType::Function: return ImColor(218, 0, 183);
		// case PinType::Delegate: return ImColor(255, 48, 48);
		}
	};

	void DrawPinIcon(const Pin& pin, bool connected, float alpha)
	{
		using IconType = ax::Drawing::IconType;

		IconType icon_type;
		ImColor color = GetIconColor(pin.type);
		color.Value.w = alpha;
		switch (pin.type) {
		case PinType::FLOW:     icon_type = IconType::Flow;   break;
		case PinType::BOOL:     icon_type = IconType::Circle; break;
		case PinType::INT:      icon_type = IconType::Circle; break;
		case PinType::FLOAT:    icon_type = IconType::Circle; break;
		case PinType::STRING:   icon_type = IconType::Circle; break;
		case PinType::TEXTURE:	icon_type = IconType::Square; break;

		// case PinType::Object:   icon_type = IconType::Circle; break;
		// case PinType::Function: icon_type = IconType::Circle; break;
		// case PinType::Delegate: icon_type = IconType::Square; break;
		case PinType::NONE:
			// pins which are drawn should not have type none
			// did you forget to set it?
			assert(false);
		default:
			// hey you forgot to say how to draw this pin type
			assert(false);
			return;
		}

		ax::Widgets::Icon(ImVec2(icon_size, icon_size), icon_type, connected, color, ImColor(.125f, .125f, .125f, alpha));
	};


}

/// ---------------------------------
/// IEventNode implementation
/// ---------------------------------

void IEventNode::GuiDraw( ed::Utilities::BlueprintNodeBuilder& builder ) {
	builder.Begin(ed::NodeId(this));

	size_t size;

	// draw the header
	// TODO header colors?
	builder.Header();
	ImGui::TextUnformatted(NodeName().data());
	ImGui::Dummy(ImVec2(0, 28));
	builder.EndHeader();

	// draw input pins
	PinInput* inputs = PinInputs(size);
	for (size_t i = 0; i < size; i++) {
		// TODO push alpha when new links are being created
		// ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);

		// a pin's pointer address is its ID for drawing purposes
		builder.Input(ed::PinId(&inputs[i]));
		
		DrawPinIcon(*inputs[i].pin, inputs[i].connection != nullptr, 1.0f);
		ImGui::Spring(0.f);
		ImGui::TextUnformatted(inputs[i].pin->name.data());
		ImGui::Spring(0.f);

		builder.EndInput();
	}

	// draw the node's center
	builder.Middle();
	// TODO how does spring really work...?
	// im::Spring(1, 0);
	GuiDrawNodeView();
	// im::Spring(1, 0);

	// draw the output pins
	PinOutput* outputs = PinOutputs(size);
	for (size_t i = 0; i < size; i++) {
		// TODO alpha again

		builder.Output(ed::PinId(&outputs[i]));

		ImGui::Spring(0.f);
		ImGui::TextUnformatted(outputs[i].pin.name.data());
		ImGui::Spring(0.f);
		DrawPinIcon(outputs[i].pin, outputs[i].connections.size() > 0, 1.0f);


		builder.EndOutput();
	}

	builder.End();
}