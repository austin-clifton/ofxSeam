#include "i-node.h"

#include "ofxImGui.h"

#include "blueprints/widgets.h"
#include "../imgui-utils/properties.h"

using namespace seam;
using namespace seam::nodes;
using namespace seam::pins;
namespace im = ImGui;
namespace ed = ax::NodeEditor;

// TODO make this configurable
float icon_size = 24.f;

namespace {
	// next two are stripped from the blueprints example in imgui-node-editor
	// TODO make the pin icons more meaningful (circles and colors are meaningless)

	ImColor GetIconColor(PinType type)
	{
		switch (type) {
		default:
		case PinType::TYPE_NONE:
		case PinType::FLOW:     return ImColor(255, 255, 255);
		case PinType::BOOL:     return ImColor(220, 48, 48);
		case PinType::INT:      return ImColor(68, 201, 156);
		case PinType::FLOAT:    return ImColor(147, 226, 74);
		case PinType::STRING:   return ImColor(124, 21, 153);
		case PinType::FBO:  return ImColor(51, 150, 215);
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
		case PinType::FLOW:		   icon_type = IconType::Flow;   break;
		case PinType::BOOL:			icon_type = IconType::Circle; break;
		case PinType::INT:			icon_type = IconType::Circle; break;
		case PinType::UINT:			icon_type = IconType::Circle; break;
		case PinType::FLOAT:		icon_type = IconType::Circle; break;
		case PinType::STRING:		icon_type = IconType::Circle; break;
		case PinType::FBO:		icon_type = IconType::Square; break;
		case PinType::MATERIAL:		icon_type = IconType::Square; break;
		case PinType::NOTE_EVENT:	icon_type = IconType::Grid; break;

		// case PinType::Object:   icon_type = IconType::Circle; break;
		// case PinType::Function: icon_type = IconType::Circle; break;
		// case PinType::Delegate: icon_type = IconType::Square; break;
		case PinType::TYPE_NONE:
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

bool INode::CompareUpdateOrder(const INode* l, const INode* r) {
	return l->update_order < r->update_order;
}

bool INode::CompareConnUpdateOrder(const NodeConnection& l, const NodeConnection& r) {
	return l.node->update_order < r.node->update_order;
}

void INode::SortParents() {
	std::sort(parents.begin(), parents.end(), &INode::CompareConnUpdateOrder);
}

bool INode::AddParent(INode* parent) {
	// the parents list is sorted to keep update traversal in order without any further sorting
	// insert to a sorted list, if this parent isn't already in the parents list
	NodeConnection conn;
	conn.node = parent;
	auto it = std::lower_bound(parents.begin(), parents.end(), conn, &INode::CompareConnUpdateOrder);
	if (it == parents.end() || it->node != parent) {
		// new parent, add the new NodeConnection for it
		conn.conn_count = 1;
		parents.insert(it, conn);
		return true;
	} else {
		// parent already exists, increase its connection count
		it->conn_count += 1;
		return false;
	}
}

bool INode::AddChild(INode* child) {
	// the children list is not sorted for now, don't think there is any reason to sort it
	auto it = std::find(children.begin(), children.end(), child);
	if (it == children.end()) {
		// new child, make a NodeConnection for it
		NodeConnection conn;
		conn.node = child;
		conn.conn_count = 1;
		children.push_back(conn);
		return true;
	} else {
		// child already exists, increase its connection count
		it->conn_count += 1;
		return false;
	}
}

void INode::SetDirty() {
	dirty = true;
	// dirtying a node dirties its children
	// need to clean up those dirty kids!
	for (size_t i = 0; i < children.size(); i++) {
		children[i].node->SetDirty();
	}
}

void INode::GuiDraw( ed::Utilities::BlueprintNodeBuilder& builder ) {
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
		builder.Input(ed::PinId((Pin*)&inputs[i]));
		
		DrawPinIcon(inputs[i], inputs[i].connection != nullptr, 1.0f);
		ImGui::Spring(0.f);
		ImGui::TextUnformatted(inputs[i].name.data());
		ImGui::Spring(0.f);

		builder.EndInput();
	}

	// draw the node's center
	builder.Middle();
	GuiDrawNodeCenter();

	// draw the output pins
	PinOutput* outputs = PinOutputs(size);
	for (size_t i = 0; i < size; i++) {
		// TODO alpha again

		builder.Output(ed::PinId(&outputs[i].pin));

		ImGui::Spring(0.f);
		ImGui::TextUnformatted(outputs[i].pin.name.data());
		ImGui::Spring(0.f);
		DrawPinIcon(outputs[i].pin, outputs[i].connections.size() > 0, 1.0f);


		builder.EndOutput();
	}

	builder.End();
}

void INode::GuiDrawNodeCenter() {
	// TODO how does spring really work...?
	// im::Spring(1, 0);
	if (gui_display_fbo != nullptr) {
		props::DrawFbo(*gui_display_fbo);
	}

	// im::Spring(1, 0);
}