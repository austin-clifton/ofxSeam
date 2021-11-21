#pragma once

#include <vector>

#include "event-nodes/event-node.h"
#include "seam/factory.h"

namespace seam {
	/// node graph editor manages the event nodes and connections that make up a seam scene
	class Editor {
	public:
		using Link = std::pair<PinInput*, PinOutput*>;

		/// to be called by ofApp::setup()
		void Setup();

		/// to be called by ofApp::draw()
		void Draw();

		/// to be called by ofApp::update()
		void Update();

		/// Uses the node factory to create a node given its node id.
		/// \param node_id the hash generated from the node's human-readable name by SCHash()
		/// \return the newly created event node, or nullptr if the NodeId didn't match a registered node type.
		IEventNode* CreateAndAdd(NodeId node_id);

		/// Helper function, uses the node factory to create a node given its human-readable name.
		/// If generating many of the same node type, you may want to hash the human name first instead.
		/// param node_name the human-readable name of the node.
		/// \return the newly created event node, or nullptr if the node name didn't match a registered node type.
		IEventNode* CreateAndAdd(const std::string_view node_name);

		bool Connect(PinInput* pin_in, PinOutput* pin_out);

		void ShowGui(bool toggle);

	private:
		static bool CompareDrawOrder(const IEventNode* l, const IEventNode* r);

		void GuiDraw();
		void GuiDrawPopups();

		// used by GUI interactions to "map" Pins to their Nodes
		struct PinToNode {
			Pin* pin;
			IEventNode* node;

			bool operator<(const PinToNode& other) {
				// compare by pointer value
				return pin < other.pin;
			}

			bool operator<(Pin* other) {
				// compare by pointer value
				return pin < other;
			}

			bool operator==(const PinToNode& other) {
				return pin == other.pin;
			}

			bool operator==(Pin* other) {
				return pin == other;
			}
		};

		ax::NodeEditor::EditorContext* node_editor_context;

		EventNodeFactory factory;

		// list of all the event nodes the graph will draw
		// this list does not need to be sorted
		std::vector<IEventNode*> nodes;

		// list of visual (texture-outputting) nodes, sorted by draw order
		std::vector<IEventNode*> visual_nodes;

		// list of all pins and their associated nodes, sorted by Pin pointer value
		std::vector<PinToNode> pins_to_nodes;

		// list of all links between pins
		std::vector<Link> links;

		// book keeping for GUI interactions
		Pin* new_link_pin;

		bool show_gui = true;
		
		bool show_create_dialog = false;
	};
}