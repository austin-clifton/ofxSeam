#ifndef PIN_H
#define PIN_H

#include <vector>

#include "ofMain.h"

#include "capnp/message.h"
#include "capnp/serialize-packed.h"
#include "seam/schema/codegen/node-graph.capnp.h"

#include "../notes.h"

namespace seam {

	// TODO MOVE ME
	enum NodePropertyType : uint8_t {
		PROP_BOOL,
		PROP_CHAR,
		PROP_FLOAT,
		PROP_INT,
		PROP_UINT,
		PROP_STRING
	};

	// TODO MOVE ME
	struct NodeProperty {
		NodeProperty(const std::string& _name, NodePropertyType _type, void* _data, size_t _count) {
			name = _name;
			type = _type;
			data = _data;
			count = _count;
		}

		std::string name;
		NodePropertyType type;
		void* data;
		size_t count;
	};

	void Deserialize(const capnp::List<PinValue, capnp::Kind::STRUCT>::Reader& serializedValues, 
		NodePropertyType type, void* dstBuff, size_t dstElementsCount);

	void Serialize(capnp::List<PinValue, capnp::Kind::STRUCT>::Builder& serializedValues,
		NodePropertyType type, void* srcBuff, size_t srcElementsCount);

	namespace nodes {
		class INode;
	}

	namespace pins {
		using PinId = uint64_t;
		using PushId = uint32_t;

		enum PinType : uint8_t {
			// invalid pin type
			// used internally but should not be used for actual pins
			TYPE_NONE,

			// flow pins are stateless triggers which just dirty any nodes that use them as inputs
			FLOW,

			// basic types
			// the output pins for these are triggers with arguments
			BOOL,
			CHAR,
			INT,
			UINT,
			FLOAT,
			STRING,

			// WTF AUSTIN WHY ARE TEXTURE AND MATERIAL DIFFERENT?????

			// FBO + resolution
			FBO,

			// a material is really just a shader with specific uniform value settings
			MATERIAL,

			// note events send a pointer to a struct from notes.h, 
			// or a struct that inherits one of those structs
			NOTE_EVENT,
		};

		NodePropertyType PinTypeToPropType(PinType pinType);
		NodePropertyType SerializedPinTypeToPropType(PinValue::Which pinType);
		PinType SerializedPinTypeToPinType(PinValue::Which pinType);

		// a bitmask enum for marking boolean properties of a Pin
		enum PinFlags : uint16_t {
			FLAGS_NONE = 0,
			///  input pins must have this flag raised;
			/// incompatible with PinFlags::OUTPUT
			INPUT = 1 << 0,

			/// output pins must have this flag raised;
			/// incompatible with PinFlags::INPUT
			OUTPUT = 1 << 1,

			/// Pins which receive feedback textures (to be used by the next update frame) must raise this flag.
			/// only INPUT pins can be marked as feedback
			FEEDBACK = 1 << 2,

			/// Only variable-sized INPUT Pins can raise this flag.
			/// Causes the node to receive a stream of pushed events to be drained during update,
			/// rather than treating the Pin's underlying vector as value channels.
			/// Is useful for receiving note events or any kind of event stream.
			EVENT_QUEUE = 1 << 3,
		};


		// TODO move me
		enum RangeType : uint8_t {
			LINEAR,
			// logarithmic
			LOG,
		};

		// base struct for pin types, holds metadata about the pin
		struct Pin {
			PinId id = 0;

			PinType type;

			// TODO make name and description string-view-ifiable again somehow

			// human-readable Pin name for display purposes
			std::string name;
			// human-readable Pin description for display purposes
			std::string description;

			// Pins keep track of the node they are associated with;
			// is expected to have a valid pointer value
			nodes::INode* node = nullptr;

			PinFlags flags = (PinFlags)0;

			virtual ~Pin() { }
		};

		class PinInput;

		// TODO pin allocs should maybe come from a pool

		// Why doesn't this just inherit Pin...?
		struct PinOutput {
			// output pins don't have values, just need to know metadata (name and type)
			// use the base struct
			Pin pin;
			std::vector<PinInput*> connections;
			// user data, if any is needed
			void* userp = nullptr;
		};

		class PinInput : public Pin {
		public:
			PinInput() {
				type = PinType::TYPE_NONE;
				channel.data = nullptr;
				channel.size = 0;
			}

			PinInput(PinType _type, 
				const std::string_view _name, 
				const std::string_view _description, 
				nodes::INode* _node,
				void* _channelsData,
				size_t _channelsSize,
				size_t _elementSizeInBytes,
				void* _pinMetadata
				)
			{
				type = _type;
				name = _name;
				description = _description;
				node = _node;
				isQueue = false;
				flags = (PinFlags)(flags | PinFlags::INPUT);
				channel.data = _channelsData;
				channel.size = _channelsSize;
				sizeInBytes = _elementSizeInBytes;
				pinMetadata = _pinMetadata;
			}

			PinInput(PinType _type,
				const std::string_view _name,
				const std::string_view _description,
				nodes::INode* _node,
				size_t _elementSizeInBytes,
				void* _pinMetadata
			) {
				type = _type;
				name = _name;
				description = _description;
				node = _node;
				isQueue = true;
				flags = (PinFlags)(flags | PinFlags::INPUT | PinFlags::EVENT_QUEUE);
				pinMetadata = _pinMetadata;
				sizeInBytes = _elementSizeInBytes;
				eventQueue.data = malloc(sizeInBytes * MAX_EVENTS);
			}

			PinInput(const std::string_view _name,
				const std::string_view _description,
				nodes::INode* _node,
				std::function<void(void)>&& callback,
				void* _pinMetadata
			)
			{
				type = PinType::FLOW;
				name = _name;
				description = _description;
				node = _node;
				isQueue = false;
				flags = (PinFlags)(flags | PinFlags::INPUT);
				pinMetadata = _pinMetadata;
				Callback = std::move(callback);
				sizeInBytes = 0;
			}

			~PinInput() {
				// Nothing to do...?
			}

			/// <summary>
			/// Get a pointer to the input pin's raw channels data.
			/// </summary>
			/// <param name="size">will be set to the size of the returned array</param>
			/// <returns>An opaque pointer to the array of channels the input Pin owns.</returns>
			void* GetChannels(size_t& size) {
				size = channel.size;
				return channel.data;
			}

			void* PinMetadata() {
				return pinMetadata;
			}

			template <typename T>
			inline void PushEvents(T* events, size_t numEvents) {
				assert(isQueue);
				size_t offset = eventQueue.size * sizeof(T);
				memcpy_s(((T*)eventQueue.data) + offset, sizeInBytes * MAX_EVENTS - offset, events, numEvents * sizeof(T));
				eventQueue.size = min(MAX_EVENTS, eventQueue.size + numEvents);
			}

			inline void* GetEvents(size_t& size) {
				assert(isQueue);
				size = eventQueue.size;
				return eventQueue.data;
			}

			inline void ClearEvents(size_t expected) {
				assert(isQueue);
				// This is just a check in case any multithreading funkiness happens;
				// Pins shouldn't be pushed to outside of an update loop, or events WILL be lost.
				assert(expected == eventQueue.size);
				memset(eventQueue.data, 0, eventQueue.size * sizeInBytes);
				eventQueue.size = 0;
			}

			inline void FlowCallback() {
				assert(type == PinType::FLOW);
				Callback();
			}

			/// push pattern id
			PushId push_id;

			// the output pin which connects to this pin
			// can be nullptr
			PinOutput* connection = nullptr;

		private:
			const size_t MAX_EVENTS = 16;

			struct Channel {
				void* data;
				size_t size;
			};

			struct EventQueue {
				void* data;
				size_t size;
			};

			bool isQueue;

			// Should be set by pin types which have GUI metadata for mins/maxes etc.
			void* pinMetadata;

			// Size of each element pointed to by void* members.
			size_t sizeInBytes;

			union {
				// Only used by non-event non-flow pins.
				Channel channel;

				// Only used by event queue inputs.
				EventQueue eventQueue;
			};

			// Only used by flow pins. Exists outside the union so the copy constructor can still exist.
			std::function<void(void)> Callback;
		};

		struct PinFloatMeta {
			PinFloatMeta(float _min = -FLT_MAX, float _max = FLT_MAX, RangeType _range = RangeType::LINEAR) {
				min = _min;
				max = _max;
				range_type = _range;
			}

			float min;
			float max;
			RangeType range_type;
		};

		struct PinIntMeta {
			PinIntMeta(int _min = INT_MIN, int _max = INT_MAX, RangeType _range = RangeType::LINEAR) {
				min = _min;
				max = _max;
				range_type = _range;
			}

			int min;
			int max;
			RangeType range_type;
		};

		struct PinUintMeta {
			uint32_t min = 0;
			uint32_t max = UINT_MAX;
			RangeType range_type = RangeType::LINEAR;
		};

		struct PinNoteEventMeta {
			/// specifies what note event types this input pin accepts
			notes::EventTypes note_types;
		};

		size_t PinTypeToElementSize(PinType type);

		PinInput SetupInputPin(
			PinType pinType,
			nodes::INode* node,
			void* channels,
			const size_t numChannels,
			const std::string_view name = "Input",
			size_t elementSizeInBytes = 0,
			void* pinMetadata = nullptr,
			const std::string_view description = ""
		);

		PinInput SetupInputQueuePin(
			PinType pinType,
			nodes::INode* node,
			const std::string_view name = "Input Queue",
			size_t elementSizeInBytes = 0,
			void* pinMetadata = nullptr,
			const std::string_view description = ""
		);

		PinInput SetupInputFlowPin(
			nodes::INode* node,
			std::function<void(void)>&& callback,
			const std::string_view name = "Flow",
			void* pinMetadata = nullptr,
			const std::string_view description = ""
		);

		PinOutput SetupOutputPin(
			nodes::INode* node, 
			PinType type, 
			std::string_view name, 
			PinFlags flags = PinFlags::FLAGS_NONE,
			void* userp = nullptr
		);

		/// Queries a linked shader program's active uniforms and creates a PinInput list from them.
		/// \param shader The linked shader program to query the uniforms of.
		/// \param node The node the shader's Pins will be added to. 
		/// \return The list of PinInputs which maps to the uniforms in the shader.
		/// All IPinInput structs are heap-allocated, and must be freed when no longer in use.
		std::vector<PinInput> UniformsToPinInputs(ofShader& shader, nodes::INode* node);
	}
};

#endif