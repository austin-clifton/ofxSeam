#pragma once

#include <vector>
#include <atomic>

#include "seam/include.h"
#include "seam/factory.h"
#include "seam/pins/push.h"
#include "seam/seamState.h"
#include "seam/pins/pin.h"
#include "seam/textureLocationResolver.h"

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

        SeamGraph();
        ~SeamGraph();

		void SetSetupParams(SetupParams params) {
			setupParams = params;
		}

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

		/// @brief Get the visual node which should be displayed in the output window.
		INode* GetVisualOutputNode();
		/// @brief Set the visual node which should be displayed in the output window.
		/// Calling this function will cause the SeamGraph to recalculate its visual update chain.
		void SetVisualOutputNode(INode* node);

		/// @brief Connect an output pin to an input pin.
		/// @return true if the Pins were successfully connected.
		bool Connect(PinInput* pinIn, PinOutput* pinOut);

		bool Disconnect(PinInput* pinIn, PinOutput* pinOut);

		void OnWindowResized(int w, int h);

        inline EventNodeFactory* GetFactory() { return &factory; }

        inline const std::vector<INode*>& GetNodes() { return nodes; }

		inline UpdateParams* GetUpdateParams() {
			updateParams.time = ofGetElapsedTimef();
			updateParams.delta_time = ofGetLastFrameTime();
			return &updateParams;
		}

    private:
		void LockAudio();

		/// @brief Recursively traverse a visual node's parent tree and update nodes in order.
		/// Also determines the draw list (but not ordering!) for this frame.
		void UpdateVisibleNodeGraph(INode* n, UpdateParams* params);

        /// @brief Recalculate a node and its children's update order
		int16_t RecalculateUpdateOrder(INode* node);

		/// @brief Recalculate the node and its children's update and/or draw orders
		void InvalidateChildren(INode* node);

		/// @brief Recalculates the update and/or draw orders of nodes
		void RecalculateTraversalOrder(INode* node);

        /// @brief Unsorted list of all the Nodes in the graph.
		std::vector<INode*> nodes;

		/// @brief These two are re-calculated each frame, and then sorted for update + draw in that order
        std::vector<INode*> nodesToDraw;

		/// @brief Visible visual nodes dictate which nodes actually get updated and drawn during those loops
		std::vector<INode*> visibleNodes;

		/// @brief Nodes which update over time need to be invalidated every frame.
		std::vector<INode*> nodesUpdateOverTime;

		/// @brief Nodes which update every frame may need to need to Update() no matter what,
		// even if they are not part of a visible visual chain
		std::vector<INode*> nodesUpdateEveryFrame;

		std::vector<IAudioNode*> audioNodes;

		/// @brief The visual node which is drawn to the output window.
		/// Dictates which Nodes are in the active visual update chain and will be updated each frame.
		INode* visualOutputNode = nullptr;

		SetupParams setupParams;

		EventNodeFactory factory;
		PushPatterns pushPatterns;
		TextureLocationResolver texLocResolver = TextureLocationResolver(&pushPatterns, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		FramePool allocPool = FramePool(8192);

		UpdateParams updateParams;

		std::atomic<bool> clearAudioNodes;
		std::atomic<bool> processingAudio;
		std::atomic<bool> audioLock;
		bool destructing = false;

        friend class seam::Editor;
    };
}