#pragma once

#include <array>
#include <string_view>
#include <iterator>
#include <atomic>
#include <string>

#include "ofMain.h"

#include "seam/flagsHelper.h"
#include "seam/properties/nodeProperty.h"
#include "seam/framePool.h"
#include "seam/seamState.h"

#include "blueprints/builders.h"
namespace ed = ax::NodeEditor;

namespace seam {
	class EventNodeFactory;
	class Editor;
	class SeamGraph;
}

namespace seam::pins {
	class PushPatterns; 
	class PinInput;
	class PinOutput;
}

namespace seam::nodes {

	using NodeId = uint64_t;

	/// A bitmask enum for marking boolean properties of a Node
	enum class NodeFlags : uint16_t {
		// visual nodes draw things using the GPU,
		// and visible visual nodes dictate each frame's update and draw workloads
		IsVisual = 1 << 0,

		// if a node uses time as an update parameter and should update every frame,
		// it should mark itself with this flag.
		// Nodes with this flag which are not part of an active visible chain will not update.
		// If you want to update regardless, use UpdatesEveryFrame.
		UpdatesOverTime = 1 << 1,

		// Nodes which receive events from external sources (MIDI for instance)
		// should mark themselves with this flag.
		// Nodes with this flag will Update() every frame, 
		// regardless of whether they are in a visual chain or not.
		UpdatesEveryFrame = 1 << 2,

		/// Nodes which process audio should use this flag and overrid INode::ProcessAudio()
		ProcessesAudio = 1 << 3,
	};

	DeclareFlagOperators(NodeFlags, uint16_t);

	struct WindowRatioFbo {
		WindowRatioFbo(
			ofFbo* _fbo, 
			pins::PinOutput* _pinOutFbo = nullptr,
			glm::vec2 _ratio = glm::vec2(1.0f),
			ofFboSettings _settings = ofFboSettings()
		) {
			fbo = _fbo;
			pinOutFbo = _pinOutFbo;
			ratio = _ratio;
			fboSettings = _settings;
		}

		ofFbo* fbo = nullptr;
		pins::PinOutput* pinOutFbo;
		/// @brief Set to (1,1) for window size, (0.5,0.5) for half screen dimensions, etc.
		glm::vec2 ratio = glm::vec2(1.0f);
		ofFboSettings fboSettings;

		// IMPROVEMENT: multi-window indexing
		// uint32_t windowIndex = 0;
	};

	struct SetupParams {
		ofSoundStreamSettings* soundSettings;
	};

	struct UpdateParams {
		float delta_time;
		float time;
		FramePool* alloc_pool;
		seam::pins::PushPatterns* push_patterns;
	};

	struct DrawParams {
		float delta_time;
		float time;
		// not sure what else is needed here yet
	};

	/// Base class for all nodes that use the eventing system
	class INode : public pins::IInPinnable, public pins::IOutPinnable {
	public:
		INode(const char* _name) 
			: node_name (_name) 
		{
			// if quickly creating and destroying nodes this may be a bad idea
			// but I don't think that's the use case for nodes...?
			instance_name = std::string(node_name);

			id = IdsDistributor::GetInstance().NextNodeId();
		}

		virtual ~INode() { }
		
		/// @brief Override this to return your own pointer to a list of input pins.
		/// @param size Should be set to the number of elements in the list pointed to by the returned pointer.
		/// @return A pointer to the array of pin inputs.
		pins::PinInput* PinInputs(size_t& size) override {
			size = 0;
			return nullptr;
		}

		/// @brief Override this to return your own pointer to a list of output pins.
		/// @param size Should be set to the number of elements in the list pointed to by the returned pointer.
		/// @return A pointer to the array of pin outputs.
		pins::PinOutput* PinOutputs(size_t& size) override {
			size = 0;
			return nullptr;
		}

		/// @brief Override to handle GPU resources initialization and any other non-trivial initialization.
		virtual void Setup(SetupParams* params) { }

		/// @brief Is called during OpenFrameworks' update() loop.
		/// Override if you need to run logic at a regular update interval.
		virtual void Update(UpdateParams* params) { }

		/// @brief Is called during OpenFrameworks' draw() loop.
		/// Override if your Node draws to an FBO.
		virtual void Draw(DrawParams* params) { }

		/// @brief Override to manage FBO resizing yourself when the window resolution changes.
		/// Generally, you should just be able to add to windowFbos instead though.
		virtual void OnWindowResized(glm::uvec2 resolution);

		/// @brief Updates a resolution pin when the window is resized, if a resolution pin exists.
		virtual bool UpdateResolutionPin(glm::uvec2 resolution);

		/// @brief Override to draw a custom GUI in the Node Inspector window using Dear ImGui.
		/// By default, this function will call GetProperties() and draw a property editor for each returned property.
		/// @return True if a value in the Node was updated and the Node needs to now be updated or re-drawn.
		virtual bool GuiDrawPropertiesList(UpdateParams* params);

		virtual void GuiDrawNodeCenter();

		/// @brief Override if you need to serialize non-pin state to maintain Node state on save and re-load.
		/// Properties returned from this function will automatically be displayed in the Node Inspector.
		/// @return A vector describing each piece of non-pin state as a NodeProperty.
		virtual std::vector<props::NodeProperty> GetProperties() {
			return std::vector<props::NodeProperty>();
		}

		inline SeamState Seam() { return seamState; }

		virtual props::NodeProperty* TryCreateProperty(const std::string& name, props::NodePropertyType type) {
			return nullptr;
		}

		void GuiDrawInputPin(ed::Utilities::BlueprintNodeBuilder& builder, pins::PinInput* inPin);
		void GuiDrawOutputPin(ed::Utilities::BlueprintNodeBuilder& builder, pins::PinOutput* outPin);
		void GuiDraw(ed::Utilities::BlueprintNodeBuilder& builder);

		inline NodeFlags Flags() {
			return flags;
		}

		/// A number which uniquely identifies this node
		inline NodeId Id() {
			return id;
		}

		inline const std::string_view NodeName() {
			return node_name;
		}

		inline const std::string& InstanceName() {
			return instance_name;
		}

		inline bool IsDirty() {
			return dirty;
		}

		inline void SetDirty() {
			dirty = true;
			// dirtying a node dirties its children
			// need to clean up those dirty kids!
			for (size_t i = 0; i < children.size(); i++) {
				children[i].node->SetDirty();
			}
		}

		inline bool UpdatesOverTime() {
			return (flags & NodeFlags::UpdatesOverTime) == NodeFlags::UpdatesOverTime;
		}

		inline bool UpdatesEveryFrame() {
			return (flags & NodeFlags::UpdatesEveryFrame) == NodeFlags::UpdatesEveryFrame;
		}

		inline bool IsVisual() {
			return (flags & NodeFlags::IsVisual) == NodeFlags::IsVisual;
		}

		pins::PinInput* FindPinInput(pins::PinId id);
		pins::PinOutput* FindPinOutput(pins::PinId id);

	protected:
		struct NodeConnection {
			INode* node = nullptr;
			// number of connections to this node,
			// since there may be more than one Pin connecting the nodes together
			uint16_t conn_count = 0;

			bool operator==(const INode* other) {
				return node == other;
			}

			bool operator<(const INode* other) {
				return node < other;
			}
		};

		virtual void InUseParents(std::vector<INode*>& out_parents) {
			out_parents.clear();
			for (size_t i = 0; i < parents.size(); i++) {
				out_parents.push_back(parents[i].node);
			}
		}

		// nodes which draw to FBOs can set this member so the FBO is drawn as part of the node's center view.
		ofFbo* gui_display_fbo = nullptr;

		// for GUI drawing and hashing, must be unique to this node **type**
		const std::string_view node_name;

		// display name, editable
		std::string instance_name;

		NodeFlags flags = (NodeFlags)0;

		// nodes' Update() calls should be made parent-first
		// update order == max(transmitters' update order) + 1
		int16_t update_order = 0;

		seam::nodes::NodeId id = 0;

		/// @brief During construction, add to this vector 
		/// to auto-track an FBO that should scale with app window resolution.
		std::vector<WindowRatioFbo> windowFbos;

	private:
		static bool CompareUpdateOrder(const INode* l, const INode* r);
		static bool CompareConnUpdateOrder(const NodeConnection& l, const NodeConnection& r);

		/// \return true if this is a new parent node
		bool AddParent(INode* parent);

		/// \return true if this is a new child node
		bool AddChild(INode* child);

		void SortParents();

		SeamState seamState;

		// a node is dirtied when its inputs change, or time progresses in some cases
		// a dirtied node needs to have its Update() called once it becomes part of the visual chain,
		// and a dirtied visual node needs to have its Draw() called
		bool dirty = true;

		/// @brief List of child nodes which this node sends events to
		std::vector<NodeConnection> children;

		/// @brief List of parent nodes which this node receives events from
		std::vector<NodeConnection> parents;		

		// the factory is a friend class so it can grab all the node's metadata easily
		friend class seam::EventNodeFactory;
		// the editor is a friend class so it can manage the node's inputs and outputs lists
		friend class seam::Editor;
		friend class seam::SeamGraph;
	};

	/// <summary>
	/// If a Node has non-static Pins, you need to inherit IDynamicIONode for pin deserialization to work as expected.
	/// </summary>
	class IDynamicPinsNode : public INode {
	public:
		struct PinInArgs {
			PinInArgs(pins::PinType _type, const std::string_view _name, size_t _totalElements, size_t _index) {
				type = _type;
				name = _name;
				totalElements = _totalElements;
				index = _index;
			}

			pins::PinType type;
			std::string_view name;
			size_t totalElements;
			size_t index;
		};

		IDynamicPinsNode(const char* name) : INode(name) {

		}

		virtual ~IDynamicPinsNode() { }

		virtual pins::PinInput* AddPinIn(PinInArgs args) = 0;
		virtual pins::PinOutput* AddPinOut(pins::PinOutput&& pinOut, size_t index) = 0;
	};

	/// <summary>
	/// If an INode processes audio, also inherit this class and override the ProcessAudio() hook. 
	/// </summary>
	class IAudioNode {
	public:
		virtual void ProcessAudio(ofSoundBuffer& input) = 0;
	};
}