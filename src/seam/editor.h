#pragma once

#include <vector>

#include "nodes/i-node.h"
#include "pins/push.h"
#include "seam/factory.h"
#include "frame-pool.h"

namespace seam {

	using namespace nodes;
	using namespace pins;

	/// node graph editor manages the event nodes and connections that make up a seam scene
	class Editor {
	public:
		/// to be called by ofApp::setup()
		void Setup(const ofSoundStreamSettings& soundSettings);

		~Editor();

		/// to be called by ofApp::draw()
		void Draw();

		void GuiDraw();

		void DrawSelectedNode();

		/// to be called by ofApp::update()
		void Update();

		void ProcessAudio(ofSoundBuffer& buffer);

		/// Uses the node factory to create a node given its node id.
		/// \param node_id the hash generated from the node's human-readable name by SCHash()
		/// \return the newly created event node, or nullptr if the NodeId didn't match a registered node type.
		INode* CreateAndAdd(seam::nodes::NodeId node_id);

		/// Helper function, uses the node factory to create a node given its human-readable name.
		/// If generating many of the same node type, you may want to hash the human name first instead.
		/// param node_name the human-readable name of the node.
		/// \return the newly created event node, or nullptr if the node name didn't match a registered node type.
		INode* CreateAndAdd(const std::string_view node_name);

		/// Connect an output pin to an input pin.
		/// \return true if the Pins were successfully connected.
		bool Connect(Pin* pin_out, Pin* pin_in);

		bool Disconnect(Pin* pin_out, Pin* pin_in);

		/// <summary>
		///  Clear the current graph.
		/// </summary>
		void NewGraph();

		seam::EventNodeFactory* GetFactory() { return factory; }
	
	private:
		struct Link {
			INode* outNode;
			PinId outPin;
			INode* inNode;
			PinId inPin;

			Link(INode* _outNode, PinId _outPin, INode* _inNode, PinId _inPin) {
				outNode = _outNode;
				outPin = _outPin;
				inNode = _inNode;
				inPin = _inPin;
			}

			inline bool operator==(const Link& other) {
				return outPin == other.outPin && inPin == other.inPin;
			}
		};

		void SaveGraph(const std::string_view filename, const std::vector<INode*>& nodesToSave);
		void LoadGraph(const std::string_view filename);

		/// recursively traverse a visual node's parent tree and update nodes in order
		/// also determines the draw list (but not ordering!) for this frame
		void UpdateVisibleNodeGraph(INode* n, UpdateParams* params);
		
		/// recalculate a node and its children's update order
		int16_t RecalculateUpdateOrder(INode* node);

		/// recalculate the node and its children's update and/or draw orders
		void InvalidateChildren(INode* node);

		/// Recalculates the update and/or draw orders of nodes
		void RecalculateTraversalOrder(INode* node);

		void GuiDrawPopups();

		ax::NodeEditor::EditorContext* nodeEditorContext = nullptr;

		seam::EventNodeFactory* factory = nullptr;

		PushPatterns push_patterns;
		FramePool alloc_pool = FramePool(8192);

		// std::vector<pins::PinId> usedPinIds;

		// list of all the event nodes the graph will draw
		// this list does not need to be sorted
		std::vector<INode*> nodes;

		// these two are re-calculated each frame, and then sorted for update + draw in that order
		std::vector<INode*> nodesToDraw;

		std::vector<IAudioNode*> audioNodes;

		// visible visual nodes dictate which nodes actually get updated and drawn during those loops
		std::vector<INode*> visibleNodes;

		// nodes which update over time need to be invalidated every frame
		std::vector<INode*> nodesUpdateOverTime;

		// nodes which update every frame need to Update() no matter what,
		// even if they are not part of a visible visual chain
		std::vector<INode*> nodesUpdateEveryFrame;

		// sorted list of all links between pins, mostly for GUI display + interactions
		std::vector<Link> links;

		// book keeping for GUI interactions
		Pin* new_link_pin;
		INode* selected_node = nullptr;

		bool show_create_dialog = false;

		std::string loaded_file;
	};
}