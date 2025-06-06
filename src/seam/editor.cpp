#include <io.h>
#include <fcntl.h>

#include "ofxImGui.h"
#include "imgui_node_editor.h"
#include "blueprints/builders.h"
#include "nfd.h"

#include "capnp/message.h"
#include "capnp/serialize-packed.h"
#include "seam/schema/codegen/node-graph.capnp.h"

#include "seam/editor.h"
#include "seam/hash.h"
#include "seam/imguiUtils/properties.h"

using namespace seam;
namespace im = ImGui;
namespace ed = ax::NodeEditor;

const char* Editor::CONFIG_FILE_NAME = "seamConfig.ini";

namespace {
	const char* POPUP_NAME_NEW_NODE = "Create New Node";
	const char* POPUP_NAME_WINDOW_RESIZE = "Resize Windows";
	const char* POPUP_NAME_NODE_CONTEXT_MENU = "Node Context Menu";
	const char* WINDOW_NAME_NODE_MENU = "Node Properties Menu";
}

Editor::~Editor() {
	std::ofstream fout(CONFIG_FILE_NAME, std::ios::trunc);
	assert(!fout == false);

	fout << "windowWidth = " << ofGetWidth() << std::endl;
	fout << "windowHeight = " << ofGetHeight() << std::endl;

	if (!loadedFile.empty()) {
		fout << "lastLoadedFile = " << loadedFile << std::endl;
		fout.close();
	}

	ed::DestroyEditor(nodeEditorContext);
}

void Editor::Setup(ofSoundStreamSettings* soundSettings) {
	SetupParams setupParams;
	setupParams.soundSettings = soundSettings;
	graph.SetSetupParams(setupParams);

	nodeEditorContext = ed::CreateEditor();

	std::string filename = iniReader.Get("", "lastLoadedFile", "");
	if (std::filesystem::exists(filename)) {
		// Make sure our node editor context is current while pre-loading
		ed::SetCurrentEditor(nodeEditorContext);
		LoadGraph(filename); 
		ed::SetCurrentEditor(nullptr);
	}

	std::string widthStr = iniReader.Get("", "windowWidth", "0");
	std::string heightStr = iniReader.Get("", "windowHeight", "0");
	int32_t width, height;
	try {
		width = std::stoi(widthStr.c_str());
		height = std::stoi(heightStr.c_str());
		if (width > 0 && height > 0) {
			ResizeWindow(width, height);
		}
	} catch (std::exception& e) {
		printf("ERROR: Invalid window width or height, could not restore from previous session");
	}
}

void Editor::Draw() {
	graph.Draw();

	// draw the selected node's display FBO if it's a visual node
	if (lastSelectedVisualNode != nullptr) {
		// visual nodes should set an FBO for GUI display!
		// TODO this assert should be placed elsewhere (it shouldn't only fire when selected)
		assert(lastSelectedVisualNode->gui_display_fbo != nullptr);
		lastSelectedVisualNode->gui_display_fbo->draw(0, 0);
	}
}

void Editor::Update() {
	graph.Update();
}

void Editor::ProcessAudio(ofSoundBuffer& buffer) {
	graph.ProcessAudio(buffer);
}

void Editor::ResizeWindow(int32_t width, int32_t height) {
	ofSetWindowShape(width, height);
	graph.OnWindowResized(width, height);
}

void Editor::GuiDrawPopups() {
	const ImVec2 open_popup_position = ImGui::GetMousePos();

	ed::Suspend();

	if (showWindowResize) {
		showWindowResize = false;
		ImGui::OpenPopup(POPUP_NAME_WINDOW_RESIZE);
	}

	ed::NodeId node_id;
	if (ed::ShowBackgroundContextMenu()) {
		showCreateDialog = true;
		ImGui::OpenPopup(POPUP_NAME_NEW_NODE);
	} else if (ed::ShowNodeContextMenu(&node_id)) {
		selectedContextMenuNode = node_id.AsPointer<INode>();
		ImGui::OpenPopup(POPUP_NAME_NODE_CONTEXT_MENU);
	}
	// TODO there are more contextual menus, see blueprints-example.cpp line 1545

	if (ImGui::BeginPopup(POPUP_NAME_NEW_NODE)) {
		// TODO specify an input or output type here if newLinkPin != nullptr
		NodeId new_node_id = graph.GetFactory()->DrawCreatePopup();
		if (new_node_id != 0) {
			showCreateDialog = false;
			INode* node = graph.CreateAndAdd(new_node_id);
			ed::SetNodePosition((ed::NodeId)node, open_popup_position);

			// TODO handle newLinkPin
			newLinkPin = nullptr;
		}

		ImGui::EndPopup();
	} else {
		showCreateDialog = false;
	}

	ImGui::SetNextWindowSize(ImVec2(250, 250));
	if (ImGui::BeginPopup(POPUP_NAME_WINDOW_RESIZE)) {
		ImGui::InputInt("Width", &windowSize.x);
		ImGui::SetItemDefaultFocus();
		ImGui::InputInt("Height", &windowSize.y);
		windowSize.x = std::max(800, windowSize.x);
		windowSize.y = std::max(600, windowSize.y);
		if (ImGui::Button("Resize") || ImGui::IsKeyDown(ImGuiKey_Enter)) {
			ResizeWindow(windowSize.x, windowSize.y);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	} else {
		showWindowResize = false;
	}

	if (ImGui::BeginPopup(POPUP_NAME_NODE_CONTEXT_MENU)) {
		// TODO HERE: copy node / copy node values buttons

		if (selectedContextMenuNode->IsVisual() && ImGui::Button("Set as Visual Output Node")) {
			graph.SetVisualOutputNode(selectedContextMenuNode);
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ed::Resume();
}

void Editor::NewGraph() {
	graph.NewGraph();

	links.clear();
	loadedFile.clear();

	selectedNode = nullptr;
	lastSelectedVisualNode = nullptr;
	newLinkPin = nullptr;
	showCreateDialog = false;
	showWindowResize = false;

	ed::ClearSelection();
}

void Editor::SaveGraph(const std::string_view filename, const std::vector<INode*>& nodes_to_save) {
	if (graph.SaveGraph(filename, nodes_to_save)) {
		loadedFile = filename;
	}
}

void Editor::LoadGraph(const std::string_view filename) {
	NewGraph();
	if (graph.LoadGraph(filename, links)) {
		loadedFile = filename;
	}	
}

void Editor::DrawSelectedNode() {
	INode* visualOutputNode = graph.GetVisualOutputNode();
	if (visualOutputNode != nullptr) {
		assert(visualOutputNode->gui_display_fbo != nullptr);
		visualOutputNode->gui_display_fbo->draw(0, 0);
	}
}

void Editor::GuiDraw() {

	im::Begin("Seam Editor", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse);

	ed::SetCurrentEditor(nodeEditorContext);

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
				if (loadedFile.empty()) {
					nfdchar_t* out_path = NULL;
					nfdresult_t result = NFD_SaveDialog(NULL, NULL, &out_path);
					if (result == NFD_OKAY) {
						SaveGraph(out_path, graph.GetNodes());
					} else if (result != NFD_CANCEL) {
						printf("NFD Error: %s\n", NFD_GetError());
					}
				} else {
					SaveGraph(loadedFile, graph.GetNodes());
				}


			}
			if (ImGui::MenuItem("New")) {
				NewGraph();
			}
			ImGui::EndMenu();
		}
		
		if (ImGui::BeginMenu("Window")) {
			if (ImGui::MenuItem("Resize Window")) {
				showWindowResize = true;
				windowSize = glm::ivec2(ofGetWidth(), ofGetHeight());
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ed::Begin("Event Flow");

	ed::Utilities::BlueprintNodeBuilder builder;
	for (auto n : graph.GetNodes()) {
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
	std::vector<ed::NodeId> selectedNodes;
	selectedNodes.resize(ed::GetSelectedObjectCount());
	int nodes_count = ed::GetSelectedNodes(selectedNodes.data(), static_cast<int>(selectedNodes.size()));
	if (nodes_count) {
		// the last selected node is the one we'll show in the properties editor
		selectedNode = selectedNodes.back().AsPointer<INode>();
		if (selectedNode->IsVisual()) {
			lastSelectedVisualNode = selectedNode;
		}
	} else {
		selectedNode = nullptr;
	}

	// if the create dialog isn't up, handle node graph interactions
	if (!showCreateDialog) {
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
				if ((pin_in->flags & PinFlags::Input) != PinFlags::Input) {
					// in and out are reversed, swap them
					std::swap(pin_in, pin_out);
				}
				assert(pin_in && pin_out);

				if ((void*)pin_in == (void*)pin_out) {
					ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
				} else if (pin_in->type != pin_out->type) {
					bool canConvert;
					pins::GetConvertSingle(pin_out->type, pin_in->type, canConvert);
					if (canConvert || pin_in->type == PinType::Any) {
						showLabel("+ Create Link", ImColor(32, 45, 32, 180));
						if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
							Connect((PinInput*)pin_in, (PinOutput*)pin_out);
						}
					} else {
						showLabel("x Can't connect pins with those differing types", ImColor(45, 32, 32, 180));
						ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
					}
				} else if (
					(pin_in->flags & PinFlags::Input) != PinFlags::Input
					|| (pin_out->flags & PinFlags::Output) != PinFlags::Output
				) {
					showLabel("x Connections must be made from input to output", ImColor(45, 32, 32, 180));
					ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);

				} else if (pin_in->node == pin_out->node) {
					showLabel("x Cannot connect input to output on the same node", ImColor(45, 32, 32, 180));
					ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
				} else {
					showLabel("+ Create Link", ImColor(32, 45, 32, 180));
					if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f)) {
						Connect((PinInput*)pin_in, (PinOutput*)pin_out);
					}
				}
			}

			// visualize potential new node 
			ed::PinId pin_id = 0;
			if (ed::QueryNewNode(&pin_id)) {
				// figure out if it's an input or output pin
				PinInput* pin_in = dynamic_cast<PinInput*>(pin_id.AsPointer<PinInput>());
				if (pin_in != nullptr) {
					newLinkPin = pin_in;
				} else {
					PinOutput* pin_out = dynamic_cast<PinOutput*>(pin_id.AsPointer<PinOutput>());
					assert(pin_out != nullptr);
					newLinkPin = pin_out;
				}
				
				if (newLinkPin) {
					showLabel("+ Create Node", ImColor(32, 45, 32, 180));
				}

				if (ed::AcceptNewItem()) {
					showCreateDialog = true;
					// I don't understand why the example does this
					// newNodeLinkPin = FindPin(pin_id);
					// newLinkPin = nullptr;
					ed::Suspend();
					ImGui::OpenPopup(POPUP_NAME_NEW_NODE);
					ed::Resume();
				}
			}
		} else {
			newLinkPin = nullptr;
		}

		ed::EndCreate();

		// visualize deletion interactions, if any
		if (ed::BeginDelete()) {
			ed::LinkId link_id = 0;
			while (ed::QueryDeletedLink(&link_id)) {
				if (ed::AcceptDeletedItem()) {
					SeamGraph::Link* link = dynamic_cast<SeamGraph::Link*>(link_id.AsPointer<SeamGraph::Link>());
					assert(link);
					auto it = std::find_if(links.begin(), links.end(), [link](const SeamGraph::Link& other) {
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
					INode* node = node_id.AsPointer<INode>();

					// Delete any related links.
					for (size_t i = links.size(); i > 0; i--) {
						auto& l = links[i - 1];
						if (l.inNode == node || l.outNode == node) {
							links.erase(links.begin() + (i - 1));
						}
					}

					graph.DeleteNode(node);
					
					// If this node is the selected node, unselect.
					if (selectedNode == node) {
						selectedNode = nullptr;
					}

					if (lastSelectedVisualNode == node) {
						lastSelectedVisualNode = nullptr;
					}
				}
			}
		}
		ed::EndDelete();
	}

	GuiDrawPopups();

	ed::End();

	ed::SetCurrentEditor(nullptr);

	im::End();

	if (selectedNode) {
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 2.f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

		im::Begin(WINDOW_NAME_NODE_MENU, nullptr, ImGuiWindowFlags_NoCollapse);
		ImGui::Text("Node: %s", selectedNode->NodeName().data());
		ImGui::Text("Update: %d", selectedNode->update_order);
		ImGui::Text("Pins:");
		bool dirty = props::DrawPinInputs(selectedNode);
		ImGui::Text("Properties:");

		dirty = selectedNode->GuiDrawPropertiesList(graph.GetUpdateParams()) || dirty;
		if (dirty) {
			selectedNode->SetDirty();
		}

		im::End();
		
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(1);
	}

}

bool Editor::Connect(PinInput* pinIn, PinOutput* pinOut) {
	bool connected = graph.Connect(pinIn, pinOut);
	assert(connected);
	links.push_back(SeamGraph::Link(pinOut->node, pinOut->id, pinIn->node, pinIn->id));
	return connected;
}

bool Editor::Disconnect(PinInput* pinIn, PinOutput* pinOut) {
	bool disconnected = graph.Disconnect(pinIn, pinOut);
	assert(disconnected);

	// Remove from links list
	SeamGraph::Link l(pinOut->node, pinOut->id, pinIn->node, pinIn->id);
	auto it = std::find(links.begin(), links.end(), l);
	assert(it != links.end());
	links.erase(it);
	return disconnected;
}

void Editor::OnWindowResized(int w, int h) {
	graph.OnWindowResized(w, h);
}