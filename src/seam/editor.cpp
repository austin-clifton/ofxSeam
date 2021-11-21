#include "editor.h"
#include "imgui/src/imgui.h"
#include "imgui/src/imgui_node_editor.h"
#include "imgui/src/blueprints/builders.h"

#include "hash.h"
#include "event-nodes/texgen-perlin.h"

using namespace seam;
namespace im = ImGui;
namespace ed = ax::NodeEditor;

namespace {
	const char* POPUP_NAME_NEW_NODE = "Create New Node";
}

bool Editor::CompareDrawOrder(const IEventNode* l, const IEventNode* r) {
	return l->draw_order < r->draw_order;
}

void Editor::Setup() {
	node_editor_context = ed::CreateEditor();

	IEventNode* perlin_noise = CreateAndAdd("Perlin Noise");
	assert(perlin_noise != nullptr);

}

void Editor::Draw() {

	// TODO update visual (texture-outputting) nodes BEFORE running GUI updates

	if (show_gui) {
		GuiDraw();
	}

	// TODO draw a final image to the screen
}

void Editor::Update() {
	// TODO nodes need to be sorted for updating!
	// Parent nodes need to propagate their changes down before a child node updates
	float time = ofGetElapsedTimef();
	for (size_t i = 0; i < nodes.size(); i++) {
		nodes[i]->Update(time);
	}
}

void Editor::GuiDrawPopups() {
	const ImVec2 open_popup_position = ImGui::GetMousePos();

	ed::Suspend();
	if (ed::ShowBackgroundContextMenu()) {
		show_create_dialog = true;
		ImGui::OpenPopup(POPUP_NAME_NEW_NODE);
	}
	// TODO there are more contextual menus, see blueprints-example.cpp line 1545

	if (ImGui::BeginPopup(POPUP_NAME_NEW_NODE)) {
		// TODO specify an input or output type here if new_link_pin != nullptr
		NodeId new_node_id = factory.DrawCreatePopup();
		if (new_node_id != 0) {
			show_create_dialog = false;
			IEventNode* node = CreateAndAdd(new_node_id);
			ed::SetNodePosition((ed::NodeId)node, open_popup_position);


			// TODO handle new_link_pin
			new_link_pin = nullptr;
		}

		ImGui::EndPopup();
	} else {
		show_create_dialog = false;
	}

	ed::Resume();
}

void Editor::GuiDraw() {
	ed::SetCurrentEditor(node_editor_context);

	ed::Begin("Event Flow");

	ed::Utilities::BlueprintNodeBuilder builder;
	for (auto n : nodes) {
		n->GuiDraw(builder);
	}

	for (const auto& l : links) {
		// TODO this runs a lot, might be better to store Pins in the Links list directly,
		// instead of PinInput* and PinOutput* which require the pointer follow to get the data we want
		ed::Link(
			(ed::LinkId)&l,
			(ed::PinId)l.first->pin,
			(ed::PinId)&l.second->pin
		);
	}

	// if the create dialog isn't up, handle node graph interactions
	if (!show_create_dialog) {
		if (ed::BeginCreate(ImColor(255, 255, 255), 2.0f)) {
			auto showLabel = [](const char* label, ImColor color) {
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
				auto size = ImGui::CalcTextSize(label);

				auto padding = ImGui::GetStyle().FramePadding;
				auto spacing = ImGui::GetStyle().ItemSpacing;

				ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

				auto rectMin = ImGui::GetCursorScreenPos() - padding;
				auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

				auto drawList = ImGui::GetWindowDrawList();
				drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
				ImGui::TextUnformatted(label);
			};

			ed::PinId start_pin_id = 0, end_pin_id = 0;
			if (ed::QueryNewLink(&start_pin_id, &end_pin_id)) {
				// the pin ID is the pointer to the Pin itself
				// figure out which Pin is the input pin, and which is the output pin
				Pin* pin_in = start_pin_id.AsPointer<Pin>();
				Pin* pin_out = end_pin_id.AsPointer<Pin>();
				if ((pin_in->flags & PinFlags::INPUT) != PinFlags::INPUT) {
					// in and out are reversed, swap them
					std::swap(pin_in, pin_out);
				}
				assert(pin_in && pin_out);

				if ((void*)pin_in == (void*)pin_out) {
					ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
				} else if (pin_in->type != pin_out->type) {
					showLabel("x Pins must be of the same type", ImColor(45, 32, 32, 180));
					ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
				} else if (
					(pin_in->flags & PinFlags::INPUT) != PinFlags::INPUT
					|| (pin_out->flags & PinFlags::OUTPUT) != PinFlags::OUTPUT
				) {
					showLabel("x Connections must be made from input to output", ImColor(45, 32, 32, 180));
					ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);

				} else {
					showLabel("+ Create Link", ImColor(32, 45, 32, 180));
					if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {

						bool connected = Connect(pin_out, pin_in);
						assert(connected);
					}
				}
			}

			ed::PinId pin_id = 0;
			if (ed::QueryNewNode(&pin_id)) {
				// figure out if it's an input or output pin
				PinInput* pin_in = dynamic_cast<PinInput*>(pin_id.AsPointer<PinInput>());
				if (pin_in != nullptr) {
					new_link_pin = pin_in->pin;
				} else {
					PinOutput* pin_out = dynamic_cast<PinOutput*>(pin_id.AsPointer<PinOutput>());
					assert(pin_out != nullptr);
					new_link_pin = &pin_out->pin;
				}
				
				if (new_link_pin) {
					showLabel("+ Create Node", ImColor(32, 45, 32, 180));
				}

				if (ed::AcceptNewItem()) {
					show_create_dialog = true;
					// I don't understand why the example does this
					// newNodeLinkPin = FindPin(pin_id);
					// newLinkPin = nullptr;
					ed::Suspend();
					ImGui::OpenPopup(POPUP_NAME_NEW_NODE);
					ed::Resume();
				}
			}
		} else {
			new_link_pin = nullptr;
		}

		ed::EndCreate();

		if (ed::BeginDelete()) {
			ed::LinkId link_id = 0;
			while (ed::QueryDeletedLink(&link_id)) {
				if (ed::AcceptDeletedItem()) {
					Link* link = dynamic_cast<Link*>(link_id.AsPointer<Link>());
					assert(link);
					auto it = std::find_if(links.begin(), links.end(), [link](const Link& other) {
						return link->first == other.first && link->second == other.second;
					});
					assert(it != links.end());

					// TODO disconnect the link from the Pins
					// remove from node_inputs and node_outputs in the nodes that the pins are attached to
					// Disconnect(link);
				}
			}

			ed::NodeId node_id = 0;
			while (ed::QueryDeletedNode(&node_id)) {
				if (ed::AcceptDeletedItem()) {

					// TODO TODO TODO

					/*auto id = std::find_if(s_Nodes.begin(), s_Nodes.end(), [nodeId](auto& node) { return node.ID == nodeId; });
					if (id != s_Nodes.end())
						s_Nodes.erase(id);*/
				}
			}
		}
		ed::EndDelete();
	}

	GuiDrawPopups();

	ed::End();
}

IEventNode* Editor::CreateAndAdd(NodeId node_id) {
	IEventNode* node = factory.Create(node_id);
	if (node != nullptr) {
		// handle book keeping for the new node

		// TODO do nodes really need to know their IDs?
		node->id = node_id;

		nodes.push_back(node);

		if ((node->Flags() & NodeFlags::IS_VISUAL) == NodeFlags::IS_VISUAL) {
			// node has no inputs yet so it must have draw order 0
			node->draw_order = 0;

			// find out where to insert the node into the sorted visual_nodes list
			auto it = std::upper_bound(visual_nodes.begin(), visual_nodes.end(), node, &Editor::CompareDrawOrder);
			// and insert it
			visual_nodes.insert(it, node);
		}

		// add input and output pins to the pins_to_nodes list
		// leaving this easy for now -- just add each pin and then sort the whole list
		// nodes probably won't be frequently created, so this doesn't need to be super fast
		size_t size;
		PinInput* inputs = node->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			PinToNode ptn;
			ptn.pin = inputs[i].pin;
			ptn.node = node;
			pins_to_nodes.push_back(ptn);
		}

		PinOutput* outputs = node->PinOutputs(size);
		for (size_t i = 0; i < size; i++) {
			PinToNode ptn;
			ptn.pin = &outputs[i].pin;
			ptn.node = node;
			pins_to_nodes.push_back(ptn);
		}

		std::sort(pins_to_nodes.begin(), pins_to_nodes.end());
	}

	return node;
}

IEventNode* Editor::CreateAndAdd(const std::string_view node_name) {
	return CreateAndAdd(SCHash(node_name.data(), node_name.length()));
}

bool Editor::Connect(IEventNode* node_out, Pin* pin_co, IEventNode* node_in, Pin* pin_ci) {
	// pin_co == pin connection out
	// pin_ci == pin connection in 
	assert((pin_ci->flags & PinFlags::INPUT) == PinFlags::INPUT);
	assert((pin_co->flags & PinFlags::OUTPUT) == PinFlags::OUTPUT);
	assert(pin_co->type == pin_ci->type);

	// find the structs that contain the pins to be connected
	// also more validations: make sure pin_out is actually an output pin of node_out, and same for the input
	PinInput* pin_in = nullptr;
	PinOutput* pin_out = nullptr;
	{
		bool found = false;
		size_t size;
		PinOutput* pin_outputs = node_out->PinOutputs(size);
		for (size_t i = 0; i < size && !found; i++) {
			pin_out = &pin_outputs[i];
			found = &pin_outputs[i].pin == pin_co;
		}
		assert(found);
		if (!found) {
			pin_out = nullptr;
		}

		found = false;
		PinInput* pin_inputs = node_in->PinInputs(size);
		for (size_t i = 0; i < size && !found; i++) {
			pin_in = &pin_inputs[i];
			found = pin_inputs[i].pin == pin_ci;
		}
		assert(found);
		assert(pin_out != nullptr && pin_in != nullptr);
		if (!found) {
			pin_in = nullptr;
		}
	}

	if (pin_in == nullptr || pin_out == nullptr) {
		return false;
	} else {
		// create the connection
		pin_out->connections.push_back(pin_in->pin);

		// add to the links list
		Link link;
		link.first = pin_in;
		link.second = pin_out;
		links.push_back(std::move(link));
		return true;
	}
}

bool Editor::Connect(Pin* pin_out, Pin* pin_in) {
	// find the node each pin is connected to
	auto it_in = std::lower_bound(pins_to_nodes.begin(), pins_to_nodes.end(), pin_in);
	auto it_out = std::lower_bound(pins_to_nodes.begin(), pins_to_nodes.end(), pin_out);
	assert(it_in != pins_to_nodes.end() && it_out != pins_to_nodes.end());

	IEventNode* node_in = it_in->node;
	IEventNode* node_out = it_out->node;

	return Connect(node_out, pin_out, node_in, pin_in);
}

void Editor::ShowGui(bool toggle) {
	show_gui = toggle;
}