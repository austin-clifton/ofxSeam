#ifndef PIN_H
#define PIN_H

#include <vector>

#include "ofMain.h"

#include "../notes.h"

namespace seam {

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

		/*
		PinInput SetupFloatInputPin(
			nodes::INode* node,
			float* channel,
			const std::string_view name = "Float Input",
			bool isQueue = false,
			PinFloatMeta* metadata = nullptr,
			const std::string_view description = ""
		) {
			return PinInput(PinType::FLOAT, name, description, node, isQueue, channel, 1, metadata);
		}

		PinInput SetupVec2InputPin(
			nodes::INode* node,
			glm::vec2* channel,
			const std::string_view name = "Vec2 Input",
			bool isQueue = false,
			PinFloatMeta* metadata = nullptr,
			const std::string_view description = ""
		) {
			return PinInput(PinType::FLOAT, name, description, node, isQueue, channel, 2, metadata);
		}

		PinInput SetupIntInputPin(
			nodes::INode* node,
			int32_t* channel,
			const std::string_view name = "Int Input",
			bool isQueue = false,
			PinFloatMeta* metadata = nullptr,
			const std::string_view description = ""
		) {
			return PinInput(PinType::INT, name, description, node, isQueue, channel, 1, metadata);
		}

		PinInput SetupIntVec2InputPin(
			nodes::INode* node,
			glm::ivec2* channel,
			const std::string_view name = "IVec2 Input",
			bool isQueue = false,
			PinFloatMeta* metadata = nullptr,
			const std::string_view description = ""
		) {
			return PinInput(PinType::INT, name, description, node, isQueue, channel, 2, metadata);
		}

		PinInput SetupUIntInputPin(
			nodes::INode* node,
			int32_t* channel,
			const std::string_view name = "UInt Input",
			bool isQueue = false,
			PinFloatMeta* metadata = nullptr,
			const std::string_view description = ""
		) {
			return PinInput(PinType::UINT, name, description, node, isQueue, channel, 1, metadata);
		}

		PinInput SetupUIntVec2InputPin(
			nodes::INode* node,
			glm::ivec2* channel,
			const std::string_view name = "UIVec2 Input",
			bool isQueue = false,
			PinFloatMeta* metadata = nullptr,
			const std::string_view description = ""
		) {
			return PinInput(PinType::UINT, name, description, node, isQueue, channel, 2, metadata);
		}
		*/



		/*
		/// Concrete implementation for IPinInput.
		/// Should be inherited by all input pin types since input pins are where pin channels data is stored.
		/// Is capable of allocating a fixed size stack allocated array, 
		/// or a variable size heap allocated array, for pin channels.
		/// Pass N == 0 for a variable sized array (see template specialization below).
		template <typename T, std::size_t N>
		class PinInput : public IPinInput {
		public:
			PinInput(const std::array<T, N>& init_vals) {
				channels = init_vals;
				flags = (PinFlags)(flags | PinFlags::INPUT);
			}

			virtual ~PinInput() { }

			/// Generic for grabbing channels data.
			/// \return a pointer to the raw data, which should be casted to the proper type based on the pin's type 
			inline void* GetChannels(size_t& size) override {
				size = channels.size();
				return channels.data();
			}

			inline T& operator[](size_t index) {
#if _DEBUG
				assert(index < channels.size());
#endif
				return channels[index];
			}

			std::array<T, N> channels;
		};

		template <typename T>
		class PinInput<T, 0> : public IPinInput {
		public:
			PinInput(const std::vector<T>& init_vals = {}) {
				// raise the event queue flag
				flags = (PinFlags)(flags | PinFlags::EVENT_QUEUE);
				channels = init_vals;
			}

			virtual ~PinInput() { }

			inline void* GetChannels(size_t& size) override {
				size = channels.size();
				return size ? &channels[0] : nullptr;
			}

			inline T& operator[](size_t index) {
				assert(index < channels.size());
				return channels[index];
			}

			std::vector<T> channels;
		};
		*/

		/*
		// these belong in their own headers, probably

		template <std::size_t N>
		struct PinBool : public PinInput<bool, N> {
			PinBool(
				std::string_view _name = "bool",
				std::string_view _description = "",
				std::array<bool, N> init_vals = { 0 },
				PinFlags _flags = PinFlags::INPUT
			
			) 
				: PinInput<bool, N>(init_vals)
			{
				name = _name;
				description = _description;
				type = PinType::BOOL;
				flags = (PinFlags)(flags | _flags);
			}
			// bool value = false;
		};

		template <std::size_t N>
		struct PinFbo : public PinInput<ofFbo*, N> {
			PinFbo(std::string_view _name = "texture", std::array<ofFbo*, N> init_vals = { 0 }) : PinInput<ofFbo*, N>(init_vals) {
				type = PinType::FBO;
				name = _name;
			}
		};

		template <std::size_t N>
		struct PinMaterial : public PinInput<ofShader*, N> {
			PinMaterial() {
				type = PinType::MATERIAL;
			}
			ofShader* shader = nullptr;
		};
		*/

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