#pragma once

#include <array>
#include <string_view>
#include <iterator>
#include <atomic>
#include <string>

#include "seam/pin.h"

#include "imgui/src/blueprints/builders.h"
namespace ed = ax::NodeEditor;

namespace seam {

	using NodeId = uint32_t;

	/// A bitmask enum for marking boolean properties of a Node
	enum NodeFlags : uint16_t {
		// visual nodes draw things using the GPU,
		// and visible visual nodes dictate each frame's update and draw workloads
		IS_VISUAL = 1 << 0,

		// if a node uses time as an update parameter it updates every frame
		// if a node wants to use time as an argument, it should mark itself with this flag
		UPDATES_OVER_TIME = 1 << 1,

		// children node of a node that updates over time also update over time,
		// but if unparented from a node that updates over time, the child can stop updating every frame too
		PARENT_UPDATES_OVER_TIME = 1 << 2,
	};

	/// Base class for all nodes that use the eventing system
	class IEventNode {
	public:
		IEventNode(const char* _name) 
			: node_name (_name) 
		{
			// if quickly creating and destroying nodes this may be a bad idea
			// but I don't think that's the use case for event nodes...?
			instance_name = std::string(node_name);
		}

		virtual ~IEventNode() { }

		virtual PinInput* PinInputs(size_t& size) = 0;

		virtual PinOutput* PinOutputs(size_t& size) = 0;

		// draw configurable and display-only properties that are internal to the node
		// and should be exposed for editing.
		virtual void GuiDrawPropertiesList() { }

		/// to be called during OF's update() loop
		virtual void Update(float time) { }

		/// to be called during OF's draw() loop
		/// should be overridden by visual nodes
		virtual void Draw() { }

		void GuiDraw( ed::Utilities::BlueprintNodeBuilder& builder );

		inline NodeFlags Flags() {
			return flags;
		}

		/// the hash number which uniquely identifies this node
		inline NodeId NodeId() {
			return id;
		}

		inline const std::string_view NodeName() {
			return node_name;
		}

		inline const std::string& InstanceName() {
			return instance_name;
		}

		void SetDirty();

		inline bool UpdatesOverTime() {
			return (flags & NodeFlags::UPDATES_OVER_TIME) || (flags & NodeFlags::PARENT_UPDATES_OVER_TIME);
		}

		inline bool IsVisual() {
			return (flags & NodeFlags::IS_VISUAL) == NodeFlags::IS_VISUAL;
		}

		void SetUpdatesOverTime(bool updates_over_time);

	protected:
		// draw the center of the node (the contextual bits)
		// should be overridden if the node's center should display something in the GUI
		virtual void GuiDrawNodeView() { }

		// for GUI drawing, must be unique to this node **type**
		const std::string_view node_name;

		// display name, editable
		std::string instance_name;

		NodeFlags flags = (NodeFlags)0;

		// a node is dirtied when its inputs change, or time progresses in some cases
		// a dirtied node needs to have its Update() called once it becomes part of the visual chain,
		// and a dirtied visual node needs to have its Draw() called
		bool dirty = true;

		// nodes' Update() calls should be made parent-first
		// update order == max(transmitters' update order) + 1
		int16_t update_order = -1;

		// visual nodes must be drawn in dependency order,
		// meaning a node using a texture from a previous node must be drawn after the previous.
		int16_t draw_order = -1;

		// TODO remove me?
		seam::NodeId id;

	private:
		struct NodeConnection {
			IEventNode* node = nullptr;
			// number of connections to this node,
			// since there may be more than one Pin connecting the nodes together
			uint16_t conn_count = 0;

			bool operator==(const IEventNode* other) {
				return node == other;
			}
		};

		// list of child nodes which this node sends events to
		std::vector<NodeConnection> children;

		// list of parent nodes which this node receives events from
		std::vector<NodeConnection> parents;

		// the factory is a friend class so it can grab all the node's metadata easily
		friend class EventNodeFactory;
		// the editor is a friend class so it can manage the node's inputs and outputs lists
		friend class Editor;
	};











	// listens to MIDI note on events for a specific MIDI note
	// transforms note on events into a percussive animation curve as a float output
	class MidiPercussive : public IEventNode {
		MidiPercussive() : IEventNode("MidiPercussive") {
			// TODO IDs 

			pin_note_on.type = PinType::NOTE_ON;
			pin_in_note_on.pin = &pin_note_on;

			pin_out_float.pin.type = PinType::FLOAT;
		}

		PinInput* PinInputs(size_t& size) override {
			size = 1;
			return &pin_in_note_on;
		}

		PinOutput* PinOutputs(size_t& size) override {
			size = 1;
			return &pin_out_float;
		}

	private:
		Pin pin_note_on;
		PinInput pin_in_note_on;
		PinOutput pin_out_float;
	};

	// this class is a little alloc heavy, use lower values for the template if you can
	template <std::size_t MaxListenersAllNotes, std::size_t MaxListenersPerNote>
	class MidiNoteBank : public IEventNode {
	public:
		MidiNoteBank() {
			// TODO pin IDs

			pin_outputs[0].pin.type = PinType::NOTE_ON;
			pin_outputs[1].pin.type = PinType::NOTE_OFF;
		}

		PinInput* PinInputs(size_t& size) override {
			size = 0; return nullptr;
		}

		PinOutput* PinOutputs(size_t& size) override {
			size = pin_outputs.size();
			return pin_outputs.data();
		}


	private:
		std::array<PinOutput, 2> pin_outputs;
	};

}