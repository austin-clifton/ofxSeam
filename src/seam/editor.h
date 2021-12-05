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

		void GuiDraw();

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

		/// Connect an output pin to an input pin.
		/// \return true if the Pins were successfully connected.
		bool Connect(IEventNode* node_out, Pin* pin_out, IEventNode* node_in, Pin* pin_in);

		/// helper function for Connect() which will find the related nodes, given that only the Pins are known
		bool Connect(Pin* pin_out, Pin* pin_in);

		bool Disconnect(IEventNode* node_out, Pin* pin_out, IEventNode* node_in, Pin* pin_in);

		bool Disconnect(Pin* pin_out, Pin* pin_in);
	
	private:
		// recursively traverse visual nodes' parent trees and determine which nodes
		// need to be drawn and/or updated this frame
		void TraverseNodesToUpdate(IEventNode* n);
		
		/// recalculate a node and its children's update order
		int16_t RecalculateUpdateOrder(IEventNode* node);

		/// recalculate a node and its children's draw order
		int16_t RecalculateDrawOrder(IEventNode* node);

		/// recalculate the node and its children's update and/or draw orders
		void InvalidateChildren(IEventNode* node, bool recalc_update, bool recalc_draw);

		/// Recalculates the update and/or draw orders of nodes
		void RecalculateTraversalOrder(IEventNode* node, bool recalc_update, bool recalc_draw);

		void GuiDrawPopups();

		inline IEventNode* MapPinToNode(Pin* pin);

		inline PinInput* FindPinInput(IEventNode* node, Pin* pin_in);

		inline PinOutput* FindPinOutput(IEventNode* node, Pin* pin_out);

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

		// these two are re-calculated each frame, and then sorted for update + draw in that order
		std::vector<IEventNode*> nodes_to_update;
		std::vector<IEventNode*> nodes_to_draw;

		// visible visual nodes dictate which nodes actually get updated and drawn during those loops
		std::vector<IEventNode*> visible_nodes;

		// list of all pins and their associated nodes, sorted by Pin pointer value
		std::vector<PinToNode> pins_to_nodes;

		// sorted list of all links between pins, mostly for GUI display + interactions
		std::vector<Link> links;

		// book keeping for GUI interactions
		Pin* new_link_pin;
		IEventNode* selected_node = nullptr;

		bool show_create_dialog = false;
	};
}