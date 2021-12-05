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
	// TODO make the pin icons more meaningful (circles and colors are meaningless)

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

bool IEventNode::CompareDrawOrder(const IEventNode* l, const IEventNode* r) {
	return l->draw_order < r->draw_order;
}

bool IEventNode::CompareUpdateOrder(const IEventNode* l, const IEventNode* r) {
	return l->update_order < r->update_order;
}

bool IEventNode::CompareConnUpdateOrder(const NodeConnection& l, const NodeConnection& r) {
	return l.node->update_order < r.node->update_order;
}

void IEventNode::SortParents() {
	std::sort(parents.begin(), parents.end(), &IEventNode::CompareConnUpdateOrder);
}

bool IEventNode::AddParent(IEventNode* parent) {
	// the parents list is sorted to keep update traversal in order without any further sorting
	// insert to a sorted list, if this parent isn't already in the parents list
	NodeConnection conn;
	conn.node = parent;
	auto it = std::lower_bound(parents.begin(), parents.end(), conn, &IEventNode::CompareConnUpdateOrder);
	if (it->node != parent) {
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

bool IEventNode::AddChild(IEventNode* child) {
	// the children list is not sorted for now, don't think there is any reason to sort it
	auto it = std::find(children.begin(), children.end(), child);
	if (it->node != child) {
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

void IEventNode::SetDirty() {
	dirty = true;
	// dirtying a node dirties its children
	// need to clean up those dirty kids!
	for (size_t i = 0; i < children.size(); i++) {
		children[i].node->SetDirty();
	}
}

void IEventNode::SetUpdatesOverTime(bool updates_over_time) {
	// check if the flag is already set
	if (updates_over_time && (flags & NodeFlags::PARENT_UPDATES_OVER_TIME) == 0) {
		// not set, set it
		flags = (NodeFlags)(flags | NodeFlags::PARENT_UPDATES_OVER_TIME);
		// children also update over time now
		for (auto child : children) {
			child.node->SetUpdatesOverTime(true);
		}

	} else if (!updates_over_time) {
		// make sure no parent nodes still update over time
		bool other_updates_over_time = false;
		for (size_t i = 0; i < parents.size() && !other_updates_over_time; i++) {
			other_updates_over_time = parents[i].node->UpdatesOverTime();
		}

		if (!other_updates_over_time) {
			// clear it
			flags = (NodeFlags)(flags & ~NodeFlags::PARENT_UPDATES_OVER_TIME);
			// if this node doesn't update over time anymore, its children might not either
			if (!UpdatesOverTime()) {
				// children nodes might not update over time now, let them sort it out
				for (auto child : children) {
					child.node->SetUpdatesOverTime(false);
				}
			}
		}
	}
}

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
		builder.Input(ed::PinId(inputs[i].pin));
		
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

		builder.Output(ed::PinId(&outputs[i].pin));

		ImGui::Spring(0.f);
		ImGui::TextUnformatted(outputs[i].pin.name.data());
		ImGui::Spring(0.f);
		DrawPinIcon(outputs[i].pin, outputs[i].connections.size() > 0, 1.0f);


		builder.EndOutput();
	}

	builder.End();
}