#include "editor.h"
#include "imgui/src/imgui.h"
#include "imgui/src/imgui_node_editor.h"
#include "imgui/src/blueprints/builders.h"

#include <io.h>
#include <fcntl.h>

#include "capnp/message.h"
#include "capnp/serialize-packed.h"
#include "node-graph.capnp.h"

#include "hash.h"
#include "imgui-utils/properties.h"
#include "nfd.h"

#include "containers/octree.h"

using namespace seam;
namespace im = ImGui;
namespace ed = ax::NodeEditor;

struct Particle {
	Particle() {

	}

	Particle(glm::vec3 pos, glm::vec3 dir = glm::vec3(0.f)) {
		position = pos;
		direction = dir;
	}
		
	glm::vec3 position;
	glm::vec3 direction;
};

namespace {
	const char* POPUP_NAME_NEW_NODE = "Create New Node";
	const char* WINDOW_NAME_NODE_MENU = "Node Properties Menu";

	void PrintGraph(const NodeGraph::Reader& reader) {
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

void TestFindItems(Octree<Particle, 8>& octree, glm::vec3 search_center, float search_radius, Particle* particles, size_t particle_count) {
	std::vector<Particle*> search_result;
	search_result.resize(particle_count / 10);
	
	auto start = std::chrono::high_resolution_clock::now();
	octree.FindItems(search_center, search_radius, search_result);
	auto end = std::chrono::high_resolution_clock::now();
	auto ns = (end - start).count();
	float ms = ns / 100000.f;
	std::cout << "took " << ms << "ms to find " << search_result.size() << " items, search radius: " << search_radius << std::endl;

	for (size_t i = 0; i < search_result.size(); i++) {
		Particle* p = search_result[i];
		// printf("fp %d: (%f, %f, %f)\n", i, p->position.x, p->position.y, p->position.z);
	}

	// Verify that the search result contains all the expected items.
	size_t expected = 0;
	for (size_t i = 0; i < particle_count; i++) {
		expected += (glm::distance(particles[i].position, search_center) <= search_radius);
	}
	assert(expected == search_result.size());
}

void Editor::Setup() {
	node_editor_context = ed::CreateEditor();

	std::function<glm::vec3(Particle*)> get = [](Particle* p) -> glm::vec3 { return p->position; };
	const float bounds = 50.f;
	Octree<Particle, 8> octree = Octree<Particle, 8>(glm::vec3(0.f), glm::vec3(bounds), get);
	const int particle_count = 100000;
	Particle* particles = new Particle[particle_count];

	for (int i = 0; i < particle_count; i++) {
		float x = (float)rand() / ((float)RAND_MAX) - 0.5f;
		float y = (float)rand() / ((float)RAND_MAX) - 0.5f;
		float z = (float)rand() / ((float)RAND_MAX) - 0.5f;

		glm::vec3 pos = glm::vec3(x, y, z);
		pos = pos * (bounds - 0.01f) * 2.f;
		particles[i] = Particle(pos);
		octree.Add(&particles[i], pos);
		assert(octree.Count() == i + 1);
	}

	// octree.PrintTree();

	TestFindItems(octree, glm::vec3(0.f), 20.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(0.f), 0.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(-10.f), 5.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(4.f, 49.f, 26.f), 20.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(33.f), 40.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(89.f), 50.f, particles, particle_count);
	TestFindItems(octree, glm::vec3(89.f), 5.f, particles, particle_count);

	for (int i = 0; i < particle_count; i++) {
		octree.Remove(&particles[i], particles[i].position);
		assert(octree.Count() == particle_count - i - 1);
	}
	
	// octree.PrintTree();

	delete[] particles;
}

void Editor::Draw() {

	DrawParams params;
	params.time = ofGetElapsedTimef();
	// not sure if this is truly delta time?
	params.delta_time = ofGetLastFrameTime();

	for (auto n : nodes_to_draw) {
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
	for (auto p : n->parents) {
		assert(last_update_order <= p.node->update_order);
		last_update_order = p.node->update_order;
		UpdateVisibleNodeGraph(p.node, params);
	}

	// now, this node can update, if it's dirty
	if (n->dirty) {
		n->Update(params);
		n->dirty = false;

		// if this is a visual node, it will need to be re-drawn now
		if (n->IsVisual()) {
			nodes_to_draw.push_back(n);
		}
	}
}

void Editor::Update() {
	// traverse the parent tree of each visible visual node and determine what needs to update
	nodes_to_draw.clear();

	// before traversing the graphs of visible nodes, dirty nodes which update every frame
	for (auto n : nodes_update_over_time) {
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
	for (auto n : nodes_update_every_frame) {
		// assume the node will dirty itself if it needs to Update()
		if (n->dirty) {
			n->Update(&params);
		}
	}

	// traverse the visible node graph and update nodes that need to be updated
	for (auto n : visible_nodes) {
		UpdateVisibleNodeGraph(n, &params);
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
		NodeId new_node_id = factory.DrawCreatePopup();
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
	nodes.clear();
	nodes_to_draw.clear();
	visible_nodes.clear();
	nodes_update_over_time.clear();
	nodes_update_every_frame.clear();
	links.clear();
	loaded_file.clear();
}

void Editor::SaveGraph(const std::string_view filename, const std::vector<INode*>& nodes_to_save) {
	// Build maps of IDs to nodes, input pins, and output pins.
	// ID assignment will happen now for anything that isn't already assigned.
	std::map<seam::nodes::NodeId, INode*> node_map;
	std::map<seam::pins::PinId, IPinInput*> input_pin_map;
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
				size_t pin_id = input_pins[i]->id;
				if (pin_id != 0) {
					if (input_pin_map.find(pin_id) != input_pin_map.end()) {
						printf("[Serialization]: Input pin has ID matching another input pin, clearing this Pin's ID. This shouldn't happen!\n");
						input_pins[i]->id = 0;
					} else {
						input_pin_map.emplace(std::make_pair(pin_id, input_pins[i]));
						max_input_pin_id = std::max(pin_id, max_input_pin_id);
					}
				} 
			}

			// Find assigned output pin IDs.
			for (size_t i = 0; i < outputs_size; i++) {
				size_t pin_id = output_pins[i].pin.id;
				if (pin_id != 0) {
					if (output_pin_map.find(pin_id) != output_pin_map.end()) {
						printf("[Serialization]: Output pin has ID matching another output pin, clearing this Pin's ID. This shouldn't happen!\n");
						output_pins[i].pin.id = 0;
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
			size_t pin_id = input_pins[i]->id;
			if (pin_id == 0) {
				while (input_pin_map.find(max_input_pin_id) != input_pin_map.end()) {
					max_input_pin_id++;
				}
				input_pins[i]->id = pin_id = max_input_pin_id;
				max_input_pin_id++;
				input_pin_map.emplace(std::make_pair(pin_id, input_pins[i]));
			}
		}

		// Assign unassigned output pins.
		for (size_t i = 0; i < outputs_size; i++) {
			size_t pin_id = output_pins[i].pin.id;
			if (pin_id == 0) {
				while (output_pin_map.find(max_output_pin_id) != output_pin_map.end()) {
					max_output_pin_id++;
				}
				output_pins[i].pin.id = pin_id = max_output_pin_id;
				max_output_pin_id++;
				output_pin_map.emplace(std::make_pair(pin_id, &output_pins[i]));
			}
		}
	}

	// Third pass: all nodes and pins should be assigned unique IDs now, and we can finally build the serialized graph.
	capnp::MallocMessageBuilder message;
	NodeGraph::Builder serialized_graph = message.initRoot<NodeGraph>();
	capnp::List<Node>::Builder serialized_nodes = serialized_graph.initNodes(nodes_to_save.size());

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

		auto inputs_builder = node_builder.initInputPins(inputs_size);
		auto outputs_builder = node_builder.initOutputPins(outputs_size);

		// Serialize input pins
		for (size_t i = 0; i < inputs_size; i++) {
			auto pin_in = input_pins[i];
			auto input_builder = inputs_builder[i];
			input_builder.setName(pin_in->name);
			input_builder.setId(pin_in->id);

			// Bail out of channel serialization if this input type doesn't have serializable state
			if (pin_in->type == PinType::FLOW
				|| pin_in->type == PinType::TEXTURE
				|| pin_in->type == PinType::MATERIAL
				|| pin_in->type == PinType::NOTE_EVENT
			) {
				continue;
			}

			size_t channels_size;
			void* channels = pin_in->GetChannels(channels_size);
			auto channels_builder = input_builder.initChannels(channels_size);

			switch (pin_in->type) {
			case PinType::BOOL: {
				bool* bool_values = (bool*)channels;
				for (size_t i = 0; i < channels_size; i++) {
					channels_builder[i].setBoolValue(bool_values[i]);
				}
				break;
			}
			case PinType::INT: {
				int32_t* int_values = (int32_t*)channels;
				for (size_t i = 0; i < channels_size; i++) {
					channels_builder[i].setIntValue(int_values[i]);
				}
				break;
			}
			case PinType::UINT: {
				uint32_t* uint_values = (uint32_t*)channels;
				for (size_t i = 0; i < channels_size; i++) {
					channels_builder[i].setUintValue(uint_values[i]);
				}
				break;
			}
			case PinType::FLOAT: {
				float* float_values = (float*)channels;
				for (size_t i = 0; i < channels_size; i++) {
					channels_builder[i].setFloatValue(float_values[i]);
				}
				break;
			}
			case PinType::CHAR:
			case PinType::STRING:
			default:
				throw std::logic_error("not implemented yet!");
			}

		}

		// Serialize output pins
		for (size_t i = 0; i < outputs_size; i++) {
			auto pin_out = output_pins[i];
			auto output_builder = outputs_builder[i];

			output_builder.setId(pin_out.pin.id);
			output_builder.setName(pin_out.pin.name);

			// Remember what connections this output has so we can serialize all the connections after this loop.
			for (size_t conn_index = 0; conn_index < pin_out.connections.size(); conn_index++) {
				auto pin_in = pin_out.connections[conn_index];
				connections.push_back(std::make_pair(pin_out.pin.id, pin_in->id));
			}
		}
	}

	// FINALLY, serialize pin connections.
	auto connections_builder = serialized_graph.initConnections(connections.size());
	for (int i = 0; i < connections.size(); i++) {
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
	auto node_graph = message.getRoot<NodeGraph>();

	PrintGraph(node_graph);
	NewGraph();

	// Keep a pin id to pin map so pins can be looked up for connecting shortly.
	std::map<PinId, Pin*> input_pin_map;
	std::map<PinId, Pin*> output_pin_map;

	// First deserialize nodes and pins
	for (const auto& serialized_node : node_graph.getNodes()) {
		auto node = CreateAndAdd(serialized_node.getNodeName().cStr());
		node->id = serialized_node.getId();
		node->instance_name = serialized_node.getDisplayName();

		auto position = ImVec2(serialized_node.getPosition().getX(), serialized_node.getPosition().getY());
		ed::SetNodePosition((ed::NodeId)node, position);

		// TODO HERE!
		// Different nodes will have different conditions for deserialization. Like Shader is gonna need to load its shader first.
		// You need to let the node do this instead, with a base class implementation that can be overridden.

		size_t inputs_size, outputs_size;
		auto input_pins = node->PinInputs(inputs_size);
		auto output_pins = node->PinOutputs(outputs_size);
		auto inputs_end = input_pins + inputs_size;
		auto outputs_end = output_pins + outputs_size;

		// Deserialize inputs
		for (const auto& serialized_pin_in : serialized_node.getInputPins()) {
			std::string_view pin_name = serialized_pin_in.getName().cStr();

			// Try to find an input pin on the node with a matching name.
			auto matchPos = std::find_if(input_pins, input_pins + inputs_size, [pin_name](const IPinInput* pin_in) 
				{ return pin_name == pin_in->name; }
			);

			if (matchPos != inputs_end) {
				IPinInput* match = *matchPos;
				match->id = serialized_pin_in.getId();

				input_pin_map.emplace(std::make_pair(match->id, match));

				size_t channels_size;
				void* channels = match->GetChannels(channels_size);

				const auto serialized_channels = serialized_pin_in.getChannels();
				if (serialized_channels.size() == 0) {
					continue;
				}

				switch (match->type) {
				case PinType::BOOL: {
					if (serialized_channels[0].isBoolValue()) {
						bool* bool_channels = (bool*)channels;
						for (size_t i = 0; i < channels_size && i < serialized_channels.size(); i++) {
							bool_channels[i] = serialized_channels[i].getBoolValue();
						}
					}
					break;
				}

				case PinType::FLOAT: {
					if (serialized_channels[0].isFloatValue()) {
						float* float_channels = (float*)channels;
						for (size_t i = 0; i < channels_size && i < serialized_channels.size(); i++) {
							float_channels[i] = serialized_channels[i].getFloatValue();
						}
					}
					break;
				}
				case PinType::INT: {
					if (serialized_channels[0].isIntValue()) {
						int* int_channels = (int32_t*)channels;
						for (size_t i = 0; i < channels_size && i < serialized_channels.size(); i++) {
							int_channels[i] = serialized_channels[i].getIntValue();
						}
					}
					break;

				}
				case PinType::UINT: {
					if (serialized_channels[0].isUintValue()) {
						uint32_t* uint_channels = (uint32_t*)channels;
						for (size_t i = 0; i < channels_size && i < serialized_channels.size(); i++) {
							uint_channels[i] = serialized_channels[i].getUintValue();
						}
					}
					break;
				}
				default:
					throw logic_error("not implemented!");
				}
			}
		}

		// Deserialize outputs
		for (const auto& serialized_pin_out : serialized_node.getOutputPins()) {
			std::string_view pin_name = serialized_pin_out.getName().cStr();

			// Try to find an input pin on the node with a matching name.
			auto match = std::find_if(output_pins, output_pins + outputs_size, [pin_name](const PinOutput& pin_out) 
				{ return pin_name == pin_out.pin.name; }
			);

			if (match != outputs_end) {
				match->pin.id = serialized_pin_out.getId();
				output_pin_map.emplace(std::make_pair(match->pin.id, &match->pin));
			}
		}
	}

	// Now add any valid connections.
	for (const auto& conn : node_graph.getConnections()) {
		auto output_pin = output_pin_map.find(conn.getOutId());
		auto input_pin = input_pin_map.find(conn.getInId());
		if (input_pin != input_pin_map.end() && output_pin != output_pin_map.end()) {
			Connect(output_pin->second, input_pin->second);
		}
	}

	loaded_file = filename;
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


	ed::SetCurrentEditor(node_editor_context);

	// remember the editor cursor's start position and the editor's window position,
	// so we can offset other draws on top of the node editor's window
	const ImVec2 editor_cursor_start_pos = ImGui::GetCursorPos();

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
			ed::PinId((Pin*)(l.first)),
			(ed::PinId)&l.second->pin
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
					showLabel("x Pins must be of the same type", ImColor(45, 32, 32, 180));
					ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
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
				IPinInput* pin_in = dynamic_cast<IPinInput*>(pin_id.AsPointer<IPinInput>());
				if (pin_in != nullptr) {
					new_link_pin = pin_in;
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

		// visualize deletion interactions, if any
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
					bool disconnected = Disconnect(&it->second->pin, it->first);
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
		ImVec2 child_size = ImVec2(256, 256);
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
	INode* node = factory.Create(node_id);
	if (node != nullptr) {
		// handle book keeping for the new node

		// TODO do nodes really need to know their IDs?
		// node->id = node_id;

		nodes.push_back(node);

		if (node->IsVisual()) {
			// probably temporary: add to the list of visible nodes up front
			auto it = std::upper_bound(visible_nodes.begin(), visible_nodes.end(), node, &INode::CompareUpdateOrder);
			visible_nodes.insert(it, node);
		}

		assert(!(node->UpdatesEveryFrame() && node->UpdatesOverTime()));

		if (node->UpdatesEveryFrame()) {
			nodes_update_every_frame.push_back(node);
		} else if (node->UpdatesOverTime()) {
			nodes_update_over_time.push_back(node);
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
	assert(pin_co->type == pin_ci->type);

	INode* parent = pin_co->node;
	INode* child = pin_ci->node;

	// find the structs that contain the pins to be connected
	// also more validations: make sure pin_out is actually an output pin of node_out, and same for the input
	IPinInput* pin_in = FindPinInput(child, pin_ci);
	PinOutput* pin_out = FindPinOutput(parent, pin_co);
	assert(pin_in && pin_out);

	if (pin_in == nullptr || pin_out == nullptr) {
		return false;
	}

	// create the connection
	pin_out->connections.push_back(pin_in);
	pin_in->connection = pin_out;

	// add to the links list
	links.push_back(Link(pin_in, pin_out));

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

bool Editor::Disconnect(Pin* pin_co, Pin* pin_ci) {
	assert((pin_ci->flags & PinFlags::INPUT) == PinFlags::INPUT);
	assert((pin_co->flags & PinFlags::OUTPUT) == PinFlags::OUTPUT);
	assert(pin_co->type == pin_ci->type);

	INode* parent = pin_co->node;
	INode* child = pin_ci->node;

	IPinInput* pin_in = FindPinInput(child, pin_ci);
	PinOutput* pin_out = FindPinOutput(parent, pin_co);
	assert(pin_in && pin_out);

	if (pin_in == nullptr || pin_out == nullptr) {
		return false;
	}

	pin_in->connection = nullptr;

	// remove from pin_out's connections list
	{
		auto it = std::find(pin_out->connections.begin(), pin_out->connections.end(), pin_in);
		assert(it != pin_out->connections.end());
		pin_out->connections.erase(it);
	}

	// remove from links list
	{
		Link l(pin_in, pin_out);
		auto it = std::find(links.begin(), links.end(), l);
		assert(it != links.end());
		links.erase(it);
	}

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

IPinInput* Editor::FindPinInput(INode* node, Pin* pin) {
	IPinInput* pin_in = nullptr;
	size_t size;
	IPinInput** pin_inputs = node->PinInputs(size);
	for (size_t i = 0; i < size && pin_in == nullptr; i++) {
		if (pin_inputs[i] == pin) {
			pin_in = pin_inputs[i];
		}
	}
	return pin_in;
}

PinOutput* Editor::FindPinOutput(INode* node, Pin* pin) {
	PinOutput* pin_out = nullptr;
	size_t size;
	PinOutput* pin_outputs = node->PinOutputs(size);
	for (size_t i = 0; i < size && pin_out == nullptr; i++) {
		if (&pin_outputs[i].pin == pin) {
			pin_out = &pin_outputs[i];
		}
	}
	return pin_out;
}

