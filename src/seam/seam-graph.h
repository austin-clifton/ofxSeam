#pragma once

#include <vector>

#include "seam/nodes/iNode.h"
#include "seam/factory.h"
#include "pins/push.h"

namespace seam {
    using namespace nodes;
    using namespace pins;

    /// @brief Responsible for managing Nodes and their connections.
    /// NOT responsible for drawing the GUI; see the Editor for GUI drawing.
    class SeamGraph {
    public:
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

        SeamGraph(const ofSoundStreamSettings& soundSettings);
        ~SeamGraph();

        /// @brief To be called during OpenFrameworks' draw() call.
        void Draw();

        /// @brief To be called during OpenFrameworks' update() call.
        void Update();

        /// @brief To be called during OpenFrameworks' hook for audio input.
        void ProcessAudio(ofSoundBuffer& buffer);

		void NewGraph();
        bool SaveGraph(const std::string_view filename, const std::vector<INode*>& nodesToSave);
		bool LoadGraph(const std::string_view filename, std::vector<Link>& outLinks);

        /// Uses the node factory to create a node given its node id.
		/// \param node_id the hash generated from the node's human-readable name by SCHash()
		/// \return the newly created event node, or nullptr if the NodeId didn't match a registered node type.
		INode* CreateAndAdd(seam::nodes::NodeId node_id);

		/// Helper function, uses the node factory to create a node given its human-readable name.
		/// If generating many of the same node type, you may want to hash the human name first instead.
		/// param node_name the human-readable name of the node.
		/// \return the newly created event node, or nullptr if the node name didn't match a registered node type.
		INode* CreateAndAdd(const std::string_view node_name);

        void DeleteNode(INode* node);

		/// @brief Connect an output pin to an input pin.
		/// @return true if the Pins were successfully connected.
		bool Connect(PinInput* pinIn, PinOutput* pinOut);

		bool Disconnect(PinInput* pinIn, PinOutput* pinOut);

		void OnWindowResized(int w, int h);

        inline EventNodeFactory* GetFactory() { return factory; }

        inline const std::vector<INode*>& GetNodes() { return nodes; }

		inline UpdateParams* GetUpdateParams() {
			updateParams.time = ofGetElapsedTimef();
			updateParams.delta_time = ofGetLastFrameTime();
			return &updateParams;
		}

    private:
		/// @brief Recursively traverse a visual node's parent tree and update nodes in order.
		/// Also determines the draw list (but not ordering!) for this frame.
		void UpdateVisibleNodeGraph(INode* n, UpdateParams* params);

        /// @brief Recalculate a node and its children's update order
		int16_t RecalculateUpdateOrder(INode* node);

		/// @brief Recalculate the node and its children's update and/or draw orders
		void InvalidateChildren(INode* node);

		/// @brief Recalculates the update and/or draw orders of nodes
		void RecalculateTraversalOrder(INode* node);

        // Unsorted list of all the Nodes in the graph.
		std::vector<INode*> nodes;

		// these two are re-calculated each frame, and then sorted for update + draw in that order
        std::vector<INode*> nodesToDraw;

		// visible visual nodes dictate which nodes actually get updated and drawn during those loops
		std::vector<INode*> visibleNodes;

		// Nodes which update over time need to be invalidated every frame.
		std::vector<INode*> nodesUpdateOverTime;

		// Nodes which update every frame may need to need to Update() no matter what,
		// even if they are not part of a visible visual chain
		std::vector<INode*> nodesUpdateEveryFrame;

		std::vector<IAudioNode*> audioNodes;

		ofSoundStreamSettings soundSettings;

		EventNodeFactory* factory = nullptr;
		PushPatterns pushPatterns;
		FramePool allocPool = FramePool(8192);

		UpdateParams updateParams;

        friend class seam::Editor;
    };
}