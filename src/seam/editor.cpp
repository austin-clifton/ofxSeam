#include "editor.h"
#include "ofxImGui.h"
#include "imgui_node_editor.h"
#include "blueprints/builders.h"

#include <io.h>
#include <fcntl.h>

#include "capnp/message.h"
#include "capnp/serialize-packed.h"
#include "seam/schema/codegen/node-graph.capnp.h"

#include "hash.h"
#include "imgui-utils/properties.h"
#include "nfd.h"

#include "seam/pins/pinConnection.h"

using namespace seam;
namespace im = ImGui;
namespace ed = ax::NodeEditor;

namespace {
	const char* POPUP_NAME_NEW_NODE = "Create New Node";
	const char* WINDOW_NAME_NODE_MENU = "Node Properties Menu";

	void PrintGraph(const seam::schema::NodeGraph::Reader& reader) {
		printf("seam graph file %s\n", reader.getName().cStr());

		// First list Nodes.
		for (const auto& node : reader.getNodes())
		{
			printf("Node %llu %s [%s]:\n", node.getId(), node.getDisplayName().cStr(), node.getNodeName().cStr());
			printf("\tPosition (%f, %f)\n", node.getPosition().getX(), node.getPosition().getY());
			printf("\tInputs:\n");
			std::stringstream ss;
			for (const auto& pin_in : node.getInputPins()) {
				ss << "\t\t" << pin_in.getId() << "\t" << pin_in.getName().cStr() << " [";
				for (const auto& chan : pin_in.getChannels()) {
					if (chan.isBoolValue()) {
						ss << chan.getBoolValue() << " ";
					} else if (chan.isFloatValue()) {
						ss << chan.getFloatValue() << " ";
					} else if (chan.isIntValue()) {
						ss << chan.getIntValue() << " ";
					} else if (chan.getUintValue()) {
						ss << chan.getUintValue() << " ";
					} else if (chan.isStringValue()) {
						ss << chan.getStringValue().cStr() << " ";
					}
				}
				ss << "]\n";
			}
			printf("%s", ss.str().c_str());

			printf("\tOutputs:\n");
			for (const auto& pin_out : node.getOutputPins()) {
				printf("\t\t%llu\t%s\n", pin_out.getId(), pin_out.getName().cStr());
			}
		}

		// Now list Connections.
		printf("\tConnections (out id, in id):\n");
		for (const auto& conn : reader.getConnections()) {
			printf("\t\t(%llu, %llu)\n", conn.getOutId(), conn.getInId());
		}
	}
}

Editor::~Editor() {
	NewGraph();

	if (factory != nullptr) {
		delete factory;
		factory = nullptr;
	}

	ed::DestroyEditor(nodeEditorContext);
}

void Editor::Setup(const ofSoundStreamSettings& soundSettings) {
	factory = new EventNodeFactory(soundSettings);
	nodeEditorContext = ed::CreateEditor();
}

void Editor::Draw() {
	DrawParams params;
	params.time = ofGetElapsedTimef();
	// not sure if this is truly delta time?
	params.delta_time = ofGetLastFrameTime();

	for (auto n : nodesToDraw) {
		n->Draw(&params);
	}

	// draw the selected node's display FBO if it's a visual node
	if (selected_node && selected_node->IsVisual()) {
		// visual nodes should set an FBO for GUI display!
		// TODO this assert should be placed elsewhere (it shouldn't only fire when selected)
		assert(selected_node->gui_display_fbo != nullptr);
		selected_node->gui_display_fbo->draw(0, 0);
	}

	// TODO draw a final image to the screen
	// probably want multiple viewports / docking?
}

void Editor::UpdateVisibleNodeGraph(INode* n, UpdateParams* params) {
	// traverse parents and update them before traversing this node
	// parents are sorted by update order, so that any "shared parents" are updated first
	int16_t last_update_order = -1;
	static std::vector<INode*> in_use_parents;
	n->InUseParents(in_use_parents);

	for (auto p : in_use_parents) {
		// assert(last_update_order <= p->update_order);
		last_update_order = p->update_order;
		UpdateVisibleNodeGraph(p, params);
	}

	// now, this node can update, if it's dirty
	if (n->dirty) {
		n->Update(params);
		n->dirty = false;

		// if this is a visual node, it will need to be re-drawn now
		if (n->IsVisual()) {
			nodesToDraw.push_back(n);
		}
	}
}

void Editor::Update() {
	// traverse the parent tree of each visible visual node and determine what needs to update
	nodesToDraw.clear();

	// before traversing the graphs of visible nodes, dirty nodes which update every frame
	for (auto n : nodesUpdateOverTime) {
		// set the dirty flag directly so children aren't affected;
		// just because the node updates over time doesn't mean it will change every frame,
		// for instance in the case of a timer which fires every XX seconds
		n->dirty = true;
	}

	// clear the per-frame allocation pool
	alloc_pool.Clear();

	// assemble update parameters
	UpdateParams params;
	params.push_patterns = &push_patterns;
	params.alloc_pool = &alloc_pool;
	params.time = ofGetElapsedTimef();
	params.delta_time = ofGetLastFrameTime();

	// traverse Nodes which must be updated every frame;
	// these are usually nodes which handle some kind of external input and/or can be dirtied by other threads
	for (auto n : nodesUpdateEveryFrame) {
		// assume the node will dirty itself if it needs to Update()
		if (n->dirty) {
			n->Update(&params);
		}
	}

	// traverse the visible node graph and update nodes that need to be updated
	for (auto n : visibleNodes) {
		UpdateVisibleNodeGraph(n, &params);
	}
}

void Editor::ProcessAudio(ofSoundBuffer& buffer) {
	for (auto n : audioNodes) {
		n->ProcessAudio(buffer);
	}
}

void Editor::GuiDrawPopups() {
	const ImVec2 open_popup_position = ImGui::GetMousePos();

	ed::Suspend();

	ed::NodeId node_id;
	if (ed::ShowBackgroundContextMenu()) {
		show_create_dialog = true;
		ImGui::OpenPopup(POPUP_NAME_NEW_NODE);
	} else if (ed::ShowNodeContextMenu(&node_id)) {
		// TODO this is a right click on the node, not left click
	}
	// TODO there are more contextual menus, see blueprints-example.cpp line 1545

	if (ImGui::BeginPopup(POPUP_NAME_NEW_NODE)) {
		// TODO specify an input or output type here if new_link_pin != nullptr
		NodeId new_node_id = factory->DrawCreatePopup();
		if (new_node_id != 0) {
			show_create_dialog = false;
			INode* node = CreateAndAdd(new_node_id);
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

void Editor::NewGraph() {
	for (size_t i = 0; i < nodes.size(); i++) {
		delete nodes[i];
	}

	nodes.clear();
	nodesToDraw.clear();
	visibleNodes.clear();
	nodesUpdateOverTime.clear();
	nodesUpdateEveryFrame.clear();
	links.clear();
	loaded_file.clear();

	IdsDistributor::GetInstance().ResetIds();
}

void Editor::SaveGraph(const std::string_view filename, const std::vector<INode*>& nodes_to_save) {
	// Build maps of IDs to nodes, input pins, and output pins.
	// ID assignment will happen now for anything that isn't already assigned.
	std::map<seam::nodes::NodeId, INode*> node_map;
	std::map<seam::pins::PinId, PinInput*> input_pin_map;
	std::map<seam::pins::PinId, PinOutput*> output_pin_map;
	seam::nodes::NodeId max_node_id = 1;
	seam::pins::PinId max_input_pin_id = 1, max_output_pin_id = 1;

	// First pass finds IDs which are already taken and finds maximum assigned IDs.
	for (size_t i = 0; i < nodes_to_save.size(); i++) {
		// If the node isn't assigned an ID yet, none of its pins will be assigned either.
		auto node_id = nodes_to_save[i]->id;
		if (node_id != 0) {
			if (node_map.find(node_id) != node_map.end()) {
				printf("[Serialization]: Node has ID matching another Node, clearing this Node's ID. This shouldn't happen!\n");
				nodes_to_save[i]->id = 0;
			} else {
				node_map.emplace(std::make_pair(node_id, nodes_to_save[i]));
			}

			max_node_id = std::max(node_id, max_node_id);

			size_t outputs_size, inputs_size;
			auto output_pins = nodes_to_save[i]->PinOutputs(outputs_size);
			auto input_pins = nodes_to_save[i]->PinInputs(inputs_size);

			// Find assigned input pin IDs.
			for (size_t i = 0; i < inputs_size; i++) {
				size_t pin_id = input_pins[i].id;
				if (pin_id != 0) {
					if (input_pin_map.find(pin_id) != input_pin_map.end()) {
						assert(0);
						printf("[Serialization]: Input pin has ID matching another input pin, clearing this Pin's ID. This shouldn't happen!\n");
						input_pins[i].id = 0;
					} else {
						input_pin_map.emplace(std::make_pair(pin_id, &input_pins[i]));
						max_input_pin_id = std::max(pin_id, max_input_pin_id);
					}
				} 
			}

			// Find assigned output pin IDs.
			for (size_t i = 0; i < outputs_size; i++) {
				size_t pin_id = output_pins[i].id;
				if (pin_id != 0) {
					if (output_pin_map.find(pin_id) != output_pin_map.end()) {
						printf("[Serialization]: Output pin has ID matching another output pin, clearing this Pin's ID. This shouldn't happen!\n");
						output_pins[i].id = 0;
					} else {
						output_pin_map.emplace(std::make_pair(pin_id, &output_pins[i]));
						max_output_pin_id = std::max(pin_id, max_output_pin_id);
					}
				}
			}
		}
	}

	// Second pass assigns IDs to anything without IDs.
	for (size_t i = 0; i < nodes_to_save.size(); i++) {
		auto node_id = nodes_to_save[i]->id;
		// Find the next available ID if unassigned.
		if (node_id == 0) {
			while (node_map.find(max_node_id) != node_map.end()) {
				max_node_id++;
			}
			nodes_to_save[i]->id = node_id = max_node_id;
			max_node_id++;
			node_map.emplace(std::make_pair(node_id, nodes_to_save[i]));
		}

		size_t outputs_size, inputs_size;
		auto output_pins = nodes_to_save[i]->PinOutputs(outputs_size);
		auto input_pins = nodes_to_save[i]->PinInputs(inputs_size);

		// Assign unassigned input pins.
		for (size_t i = 0; i < inputs_size; i++) {
			assert(false);
			size_t pin_id = input_pins[i].id;
			assert(pin_id != 0);
			if (pin_id == 0) {
				while (input_pin_map.find(max_input_pin_id) != input_pin_map.end()) {
					max_input_pin_id++;
				}
				input_pins[i].id = pin_id = max_input_pin_id;
				max_input_pin_id++;
				input_pin_map.emplace(std::make_pair(pin_id, &input_pins[i]));
			}
		}

		// Assign unassigned output pins.
		for (size_t i = 0; i < outputs_size; i++) {
			assert(false);
			size_t pin_id = output_pins[i].id;
			if (pin_id == 0) {
				while (output_pin_map.find(max_output_pin_id) != output_pin_map.end()) {
					max_output_pin_id++;
				}
				output_pins[i].id = pin_id = max_output_pin_id;
				max_output_pin_id++;
				output_pin_map.emplace(std::make_pair(pin_id, &output_pins[i]));
			}
		}
	}

	// Third pass: all nodes and pins should be assigned unique IDs now, and we can finally build the serialized graph.
	capnp::MallocMessageBuilder message;
	seam::schema::NodeGraph::Builder serialized_graph = message.initRoot<seam::schema::NodeGraph>();
	capnp::List<seam::schema::Node>::Builder serialized_nodes = serialized_graph.initNodes(nodes_to_save.size());

	// Pair is output pin, input pin
	std::vector<std::pair<PinId, PinId>> connections;

	for (size_t i = 0; i < nodes_to_save.size(); i++) {
		auto node = nodes_to_save[i];
		auto node_builder = serialized_nodes[i];

		// Serialize node fields.
		auto nodePosition = ed::GetNodePosition((ed::NodeId)node);
		node_builder.getPosition().setX(nodePosition.x);
		node_builder.getPosition().setY(nodePosition.y);

		node_builder.setDisplayName(node->InstanceName());
		node_builder.setNodeName(node->NodeName().data());
		node_builder.setId(node->Id());

		size_t outputs_size, inputs_size;
		auto output_pins = nodes_to_save[i]->PinOutputs(outputs_size);
		auto input_pins = nodes_to_save[i]->PinInputs(inputs_size);
		const auto properties = node->GetProperties();

		auto inputs_builder = node_builder.initInputPins(inputs_size);
		auto outputs_builder = node_builder.initOutputPins(outputs_size);
		auto propertiesBuilder = node_builder.initProperties(properties.size());

		// Serialize input pins
		for (size_t i = 0; i < inputs_size; i++) {
			auto& pin_in = input_pins[i];
			auto input_builder = inputs_builder[i];
			input_builder.setName(pin_in.name);
			input_builder.setId(pin_in.id);

			// Bail out of channel serialization if this input type doesn't have serializable state
			if (pin_in.type == PinType::FLOW
				|| pin_in.type == PinType::FBO
				|| pin_in.type == PinType::MATERIAL
				|| pin_in.type == PinType::NOTE_EVENT
			) {
				continue;
			}

			size_t channels_size;
			void* channels = pin_in.GetChannels(channels_size);
			auto channels_builder = input_builder.initChannels(channels_size);

			Serialize(channels_builder, PinTypeToPropType(pin_in.type), channels, channels_size);
		}

		// Serialize output pins
		for (size_t i = 0; i < outputs_size; i++) {
			auto pin_out = output_pins[i];
			auto output_builder = outputs_builder[i];

			output_builder.setId(pin_out.id);
			output_builder.setName(pin_out.name);

			// Remember what connections this output has so we can serialize all the connections after this loop.
			for (size_t conn_index = 0; conn_index < pin_out.connections.size(); conn_index++) {
				auto& conn = pin_out.connections[conn_index];
				connections.push_back(std::make_pair(pin_out.id, conn.input->id));
			}
		}

		// Serialize properties
		for (size_t i = 0; i < properties.size(); i++) {
			const auto prop = properties[i];
			auto propBuilder = propertiesBuilder[i];

			propBuilder.setName(prop.name);
			auto valuesBuilder = propBuilder.initValues(prop.count);

			Serialize(valuesBuilder, prop.type, prop.data, prop.count);
		}
	}

	// FINALLY, serialize pin connections.
	auto connections_builder = serialized_graph.initConnections(connections.size());
	for (size_t i = 0; i < connections.size(); i++) {
		connections_builder[i].setOutId(connections[i].first);
		connections_builder[i].setInId(connections[i].second);
	}

	// TODO get a name from elsewhere
	serialized_graph.setName(filename.data());

	PrintGraph(serialized_graph.asReader());

	FILE* file = fopen(filename.data(), "wb");
	if (file == NULL) {
		printf("error while opening %s: %d", filename.data(), errno);
	} else {
		capnp::writePackedMessageToFd(fileno(file), message);
		fclose(file);
		loaded_file = filename;
	}
}

void Editor::LoadGraph(const std::string_view filename) {
	FILE* file = fopen(filename.data(), "rb");
	if (file == NULL) {
		printf("Error opening file for read back: %d\n", errno);
		return;
	}
	auto message = capnp::PackedFdMessageReader(fileno(file));
	auto node_graph = message.getRoot<seam::schema::NodeGraph>();

	PrintGraph(node_graph);
	NewGraph();

	// Keep a pin id to pin map so pins can be looked up for connecting shortly.
	std::map<PinId, Pin*> input_pin_map;
	std::map<PinId, Pin*> output_pin_map;

	// Remember max pin and node IDs.
	PinId maxPinId = 0;
	NodeId maxNodeId = 0;

	// First deserialize nodes and pins
	for (const auto& serialized_node : node_graph.getNodes()) {
		auto node = CreateAndAdd(serialized_node.getNodeName().cStr());
		node->id = serialized_node.getId();
		maxNodeId = std::max(node->id, maxNodeId);
		node->instance_name = serialized_node.getDisplayName();

		auto position = ImVec2(serialized_node.getPosition().getX(), serialized_node.getPosition().getY());
		ed::SetNodePosition((ed::NodeId)node, position);

		// TODO HERE!
		// Different nodes will have different conditions for deserialization. Like Shader is gonna need to load its shader first.
		// You need to let the node do this instead, with a base class implementation that can be overridden.

		size_t inputs_size, outputs_size;
		auto input_pins = node->PinInputs(inputs_size);
		auto output_pins = node->PinOutputs(outputs_size);

		auto dynamicPinsNode = dynamic_cast<IDynamicPinsNode*>(node);

		// Deserialize inputs
		for (const auto& serialized_pin_in : serialized_node.getInputPins()) {
			std::string serialized_pin_name = serialized_pin_in.getName().cStr();
			std::transform(serialized_pin_name.begin(), serialized_pin_name.end(), serialized_pin_name.begin(), 
				[](unsigned char c) {return std::tolower(c); });

			// Try to find an input pin on the node with a matching name.
			auto matchPos = std::find_if(input_pins, input_pins + inputs_size, [serialized_pin_name](const PinInput& pin_in) {
				std::string name_copy = pin_in.name;
				std::transform(name_copy.begin(), name_copy.end(), name_copy.begin(), [](unsigned char c) {return std::tolower(c); });
				return name_copy == serialized_pin_name;
			});

			const auto serialized_channels = serialized_pin_in.getChannels();

			if (matchPos != (input_pins + inputs_size)) {
				PinInput* match = matchPos;
				match->id = serialized_pin_in.getId();
				maxPinId = std::max(match->id, maxPinId);

				input_pin_map.emplace(std::make_pair(match->id, match));

				if (serialized_channels.size()) {
					size_t channels_size;
					void* channels = match->GetChannels(channels_size);

					Deserialize(serialized_channels, PinTypeToPropType(match->type), channels, channels_size);
				}

			} else if (dynamicPinsNode != nullptr) {
				PinType pinType = SerializedPinTypeToPinType(serialized_channels[0].which());
				PinInput* added = dynamicPinsNode->AddPinIn(pinType, serialized_pin_name,
					PinTypeToElementSize(pinType), serialized_channels.size());

				if (added == nullptr) {
					printf("Dynamic Pins Node %s refused to add a pin named %s\n",
						node->NodeName().data(), serialized_pin_name.c_str());
				} else {
					// Refresh the input pins list!
					added->id = serialized_pin_in.getId();
					input_pins = node->PinInputs(inputs_size);
					input_pin_map.emplace(std::make_pair(added->id, added));
				}

			} else {
				printf("Could not match serialized input pin with name %s on node %s\n", 
					serialized_pin_name.c_str(), serialized_node.getDisplayName().cStr());
			}
		}

		// Deserialize outputs
		for (const auto& serialized_pin_out : serialized_node.getOutputPins()) {
			std::string_view pin_name = serialized_pin_out.getName().cStr();

			// Try to find an input pin on the node with a matching name.
			auto match = std::find_if(output_pins, output_pins + outputs_size, [pin_name](const PinOutput& pin_out) 
				{ return pin_name == pin_out.name; }
			);

			if (match != (output_pins + outputs_size)) {
				match->id = serialized_pin_out.getId();
				maxPinId = std::max(match->id, maxPinId);
				output_pin_map.emplace(std::make_pair(match->id, (Pin*)match));
			} else if (dynamicPinsNode != nullptr) {
				// TODO....
				throw std::runtime_error("oh shit I should implement this.....");

			} else {
				printf("Could not match serialized output pin with name %s on node %s\n",
					pin_name.data(), serialized_node.getDisplayName().cStr());
			}
		}

		// Deserialize properties
		for (const auto& serializedProperty : serialized_node.getProperties()) {
			const auto values = serializedProperty.getValues();

			if (!values.size()) {
				continue;
			}

			NodePropertyType propertyType = pins::SerializedPinTypeToPropType(values[0].which());

			// First try to match the property against a known property in the Node's properties list.
			// Ask for the properties list again after deserializing each property in case the list changed.
			auto properties = node->GetProperties();
			for (auto& prop : properties) {
				if (prop.name == serializedProperty.getName().cStr()) {
					Deserialize(values, propertyType, prop.data, prop.count);
				}
			}

			// No match, the property could be dynamic though. Ask the Node to create it.
			NodeProperty* prop = node->TryCreateProperty(serializedProperty.getName().cStr(), propertyType);
			if (prop != nullptr) {
				Deserialize(values, propertyType, prop->data, prop->count);
			}
			else {
				printf("Serialized Node property %s could not be matched against a known property on Node %s.\n",
					serializedProperty.getName().cStr(), node->NodeName().data());
			}
		}
	}

	// Now add any valid connections.
	for (const auto& conn : node_graph.getConnections()) {
		auto output_pin = output_pin_map.find(conn.getOutId());
		auto input_pin = input_pin_map.find(conn.getInId());
		if (input_pin != input_pin_map.end() && output_pin != output_pin_map.end()) {
			if (input_pin->second->type != output_pin->second->type) {
				continue;
			}
			
			Connect(output_pin->second, input_pin->second);
		}
	}

	ed::NavigateToContent();

	// Make sure the IdsDistributor knows where to start assigning IDs from.
	IdsDistributor::GetInstance().ResetIds();
	IdsDistributor::GetInstance().SetNextNodeId(maxNodeId + 1);
	IdsDistributor::GetInstance().SetNextPinId(maxPinId + 1);

	loaded_file = filename;
}

void Editor::DrawSelectedNode() {
	// draw the selected node's display FBO if it's a visual node
	if (selected_node && selected_node->IsVisual()) {
		// visual nodes should set an FBO for GUI display!
		// TODO this assert should be placed elsewhere (it shouldn't only fire when selected)
		assert(selected_node->gui_display_fbo != nullptr);
		selected_node->gui_display_fbo->draw(0, 0);
	}
}

void Editor::GuiDraw() {

	im::Begin("Seam Editor", nullptr, ImGuiWindowFlags_MenuBar);

	if (ImGui::BeginMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open...", "Ctrl+O")) {
				nfdchar_t* out_path = NULL;
				nfdresult_t result = NFD_OpenDialog(NULL, NULL, &out_path);
				if (result == NFD_OKAY) {
					LoadGraph(out_path);
				} else if (result == NFD_CANCEL) {
					// Nothing to do...
				} else {
					printf("NFD Error: %s\n", NFD_GetError());
				}
			}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				if (loaded_file.empty()) {
					nfdchar_t* out_path = NULL;
					nfdresult_t result = NFD_SaveDialog(NULL, NULL, &out_path);
					if (result == NFD_OKAY) {
						SaveGraph(out_path, nodes);
					} else if (result != NFD_CANCEL) {
						printf("NFD Error: %s\n", NFD_GetError());
					}
				} else {
					SaveGraph(loaded_file, nodes);
				}


			}
			/*
			if (ImGui::MenuItem("Save Selected")) {
				ImGui::BeginPopup
			}
			*/
			if (ImGui::MenuItem("New")) {
				NewGraph();
			}
			ImGui::EndMenu();
		}


		ImGui::EndMenuBar();
	}


	ed::SetCurrentEditor(nodeEditorContext);

	// remember the editor cursor's start position and the editor's window position,
	// so we can offset other draws on top of the node editor's window
	const ImVec2 editor_cursor_start_pos = ImGui::GetCursorPos();

	ed::Begin("Event Flow");

	ed::Utilities::BlueprintNodeBuilder builder;
	for (auto n : nodes) {
		n->GuiDraw(builder);
	}

	for (const auto& l : links) {
		// Look up each pin on the Node.
		// Pins aren't cached, because their pointers might change.
		pins::PinInput* pinIn = l.inNode->FindPinInput(l.inPin);
		pins::PinOutput* pinOut = l.outNode->FindPinOutput(l.outPin);

		ed::Link(
			(ed::LinkId)&l,
			(ed::PinId)(pinIn),
			(ed::PinId)(pinOut)
		);
	}

	// query if node(s) have been selected with left click
	// the last selected node should be shown in the properties editor
	std::vector<ed::NodeId> selected_nodes;
	selected_nodes.resize(ed::GetSelectedObjectCount());
	int nodes_count = ed::GetSelectedNodes(selected_nodes.data(), static_cast<int>(selected_nodes.size()));
	if (nodes_count) {
		// the last selected node is the one we'll show in the properties editor
		selected_node = selected_nodes.back().AsPointer<INode>();
	} else {
		selected_node = nullptr;
	}

	// if the create dialog isn't up, handle node graph interactions
	if (!show_create_dialog) {
		// are we trying to create a new pin or node connection? if so, visualize it
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

			// visualize potential new links
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
					bool canConvert;
					pins::GetConvertSingle(pin_out->type, pin_in->type, canConvert);
					if (canConvert || pin_in->type == PinType::ANY) {
						showLabel("+ Create Link", ImColor(32, 45, 32, 180));
						if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
							bool connected = Connect(pin_out, pin_in);
							assert(connected);
						}
					} else {
						showLabel("x Can't connect pins with those differing types", ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
					}
				} else if (
					(pin_in->flags & PinFlags::INPUT) != PinFlags::INPUT
					|| (pin_out->flags & PinFlags::OUTPUT) != PinFlags::OUTPUT
				) {
					showLabel("x Connections must be made from input to output", ImColor(45, 32, 32, 180));
					ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);

				} else if (pin_in->node == pin_out->node) {
					showLabel("x Cannot connect input to output on the same node", ImColor(45, 32, 32, 180));
					ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
				} else {
					showLabel("+ Create Link", ImColor(32, 45, 32, 180));
					if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
						bool connected = Connect(pin_out, pin_in);
						assert(connected);
					}
				}
			}

			// visualize potential new node 
			ed::PinId pin_id = 0;
			if (ed::QueryNewNode(&pin_id)) {
				// figure out if it's an input or output pin
				PinInput* pin_in = dynamic_cast<PinInput*>(pin_id.AsPointer<PinInput>());
				if (pin_in != nullptr) {
					new_link_pin = pin_in;
				} else {
					PinOutput* pin_out = dynamic_cast<PinOutput*>(pin_id.AsPointer<PinOutput>());
					assert(pin_out != nullptr);
					new_link_pin = pin_out;
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

		// visualize deletion interactions, if any
		if (ed::BeginDelete()) {
			ed::LinkId link_id = 0;
			while (ed::QueryDeletedLink(&link_id)) {
				if (ed::AcceptDeletedItem()) {
					Link* link = dynamic_cast<Link*>(link_id.AsPointer<Link>());
					assert(link);
					auto it = std::find_if(links.begin(), links.end(), [link](const Link& other) {
						return link->inPin == other.inPin && link->outPin == other.outPin;
					});
					assert(it != links.end());
					PinInput* pinIn = it->inNode->FindPinInput(it->inPin);
					PinOutput* pinOut = it->outNode->FindPinOutput(it->outPin);
					assert(pinIn != nullptr && pinOut != nullptr);
					bool disconnected = Disconnect(pinIn, pinOut);
					assert(disconnected);
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

	if (selected_node) {
		
		ImVec2 window_size = im::GetContentRegionAvail();
		ImVec2 window_pos = im::GetWindowPos();
		ImVec2 child_size = ImVec2(512, 256);
		im::SetNextWindowPos(ImVec2(
			window_pos.x + window_size.x,
			editor_cursor_start_pos.y + window_pos.y)
			// add padding
			+ ImVec2(-8.f, 8.f),
			0,
			ImVec2(1.f, 0.f)
		);

		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

		if (im::BeginChild(WINDOW_NAME_NODE_MENU, child_size, true)) {
			ImGui::Text("Update: %d", selected_node->update_order);
			ImGui::Text("Pins:");
			bool dirty = props::DrawPinInputs(selected_node);
			ImGui::Text("Properties:");
			dirty = selected_node->GuiDrawPropertiesList() || dirty;
			if (dirty) {
				selected_node->SetDirty();
			}
		}
		im::EndChild();
		
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(1);

	}


	im::End();
}

INode* Editor::CreateAndAdd(NodeId node_id) {
	INode* node = factory->Create(node_id);
	if (node != nullptr) {
		nodes.push_back(node);

		if (node->IsVisual()) {
			// probably temporary: add to the list of visible nodes up front
			auto it = std::upper_bound(visibleNodes.begin(), visibleNodes.end(), node, &INode::CompareUpdateOrder);
			visibleNodes.insert(it, node);
		}

		assert(!(node->UpdatesEveryFrame() && node->UpdatesOverTime()));

		if (node->UpdatesEveryFrame()) {
			nodesUpdateEveryFrame.push_back(node);
		} else if (node->UpdatesOverTime()) {
			nodesUpdateOverTime.push_back(node);
		}

		// Does this Node process audio?
		IAudioNode* audioNode = dynamic_cast<IAudioNode*>(node);
		if (audioNode != nullptr) {
			audioNodes.push_back(audioNode);
		}
	}

	return node;
}

INode* Editor::CreateAndAdd(const std::string_view node_name) {
	return CreateAndAdd(SCHash(node_name.data(), node_name.length()));
}

bool Editor::Connect(Pin* pin_co, Pin* pin_ci) {
	// pin_co == pin connection out
	// pin_ci == pin connection in
	// names are hard :(
	assert((pin_ci->flags & PinFlags::INPUT) == PinFlags::INPUT);
	assert((pin_co->flags & PinFlags::OUTPUT) == PinFlags::OUTPUT);

	INode* parent = pin_co->node;
	INode* child = pin_ci->node;

	// find the structs that contain the pins to be connected
	// also more validations: make sure pin_out is actually an output pin of node_out, and same for the input
	PinInput* pin_in = child->FindPinInput(pin_ci->id);
	PinOutput* pin_out = parent->FindPinOutput(pin_co->id);
	assert(pin_in && pin_out);

	if (pin_in == nullptr || pin_out == nullptr) {
		return false;
	}

	// create the connection
	pin_out->connections.push_back(PinConnection(pin_in, pin_out->type));
	pin_in->connection = pin_out;

	// add to the links list
	links.push_back(Link(parent, pin_co->id, child, pin_ci->id));

	parent->OnPinConnected(pin_in, pin_out);
	child->OnPinConnected(pin_in, pin_out);

	// give the input pin the default push pattern
	pin_in->push_id = push_patterns.Default().id;

	// add to each node's parents and children list
	// if this connection rearranged the node graph, 
	// its traversal order will need to be recalculated
	const bool is_new_child = parent->AddChild(child);
	const bool is_new_parent = child->AddParent(parent);
	const bool rearranged = is_new_parent || is_new_child;

	if (rearranged) {
		RecalculateTraversalOrder(child);
	}

	return true;
}

bool Editor::Disconnect(Pin* pinIn, Pin* pinOut) {
	assert((pinIn->flags & PinFlags::INPUT) == PinFlags::INPUT);
	assert((pinOut->flags & PinFlags::OUTPUT) == PinFlags::OUTPUT);

	INode* parent = pinOut->node;
	INode* child = pinIn->node;

	PinInput* pin_in = child->FindPinInput(pinIn->id);
	PinOutput* pin_out = parent->FindPinOutput(pinOut->id);
	assert(pin_in && pin_out);

	if (pin_in == nullptr || pin_out == nullptr) {
		return false;
	}

	pin_in->connection = nullptr;

	// remove from pin_out's connections list
	size_t i = 0;
	for (; i < pin_out->connections.size(); i++) {
		if (pin_out->connections[i].input == pin_in) {
			break;
		}
	}
	assert(i < pin_out->connections.size());
	pin_out->connections.erase(pin_out->connections.begin() + i);

	// remove from links list
	{
		Link l(parent, pin_out->id, child, pin_in->id);
		auto it = std::find(links.begin(), links.end(), l);
		assert(it != links.end());
		links.erase(it);
	}

	parent->OnPinDisconnected(pin_in, pin_out);
	child->OnPinDisconnected(pin_in, pin_out);

	// track if we need to update traversal order
	bool rearranged = false;

	// remove or decrement from receivers / transmitters lists
	{
		auto it = std::find(parent->children.begin(), parent->children.end(), child);
		assert(it != parent->children.end());
		if (it->conn_count == 1) {
			parent->children.erase(it);
			rearranged = true;
		} else {
			it->conn_count -= 1;
		}
	}

	{
		auto it = std::find(child->parents.begin(), child->parents.end(), parent);
		assert(it != child->parents.end());
		if (it->conn_count == 1) {
			child->parents.erase(it);
			rearranged = true;
		} else {
			it->conn_count -= 1;
		}
	}

	if (rearranged) {
		// the child node and its children need to recalculate draw and update order now
		RecalculateTraversalOrder(child);
	}

	return true;
}

int16_t Editor::RecalculateUpdateOrder(INode* node) {
	// update order is always max of parents' update order + 1
	if (node->update_order != -1) {
		return node->update_order;
	}

	// TODO! deal with feedback pins

	int16_t max_parents_update_order = -1;
	for (auto parent : node->parents) {
		max_parents_update_order = std::max(
			max_parents_update_order,
			// TODO checking the parent update order may not be necessary,
			// if the children list is sorted for its recursive traversal
			RecalculateUpdateOrder(parent.node)
		);
	}
	node->update_order = max_parents_update_order + 1;

	// recursively update children
	for (auto child : node->children) {
		RecalculateUpdateOrder(child.node);
	}

	return node->update_order;
}

void Editor::InvalidateChildren(INode* node) {
	{
		// if this node has already been invalidated, don't go over it again
		if (node->update_order == -1) {
			return;
		}
	}

	node->update_order = -1;
	for (auto child : node->children) {
		InvalidateChildren(child.node);
	}
}

void Editor::RecalculateTraversalOrder(INode* node) {
	// invalidate this node and its children
	InvalidateChildren(node);
	RecalculateUpdateOrder(node);
}
