#pragma once

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
			TEXTURE,

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


		struct Texture {
			glm::ivec2 resolution;
			ofFbo* fbo = nullptr;
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
		};

		// TODO pin allocs should maybe come from a pool

		class IPinInput;

		// Why doesn't this just inherit Pin...?
		struct PinOutput {
			// output pins don't have values, just need to know metadata (name and type)
			// use the base struct
			Pin pin;
			std::vector<IPinInput*> connections;
			// user data, if any is needed
			void* userp = nullptr;
		};

		/// Abstract class for input pins.
		/// Defines a generic function to retrieve the channels array which must be implemented.
		class IPinInput : public Pin {
		public:
			virtual ~IPinInput() { }

			/// Get a pointer to the input pin's raw channels data.
			/// \param size should be set to the size of the returned array
			/// \return the c-style array of channels the input Pin owns
			virtual void* GetChannels(size_t& size) = 0;

			/// push pattern id
			PushId push_id;

			// the output pin which connects to this pin
			// can be nullptr
			PinOutput* connection = nullptr;
		};

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
		struct PinTexture : public PinInput<Texture, N> {
			PinTexture() {
				type = PinType::TEXTURE;
			}
			Texture value;
		};

		template <std::size_t N>
		struct PinMaterial : public PinInput<ofShader*, N> {
			PinMaterial() {
				type = PinType::MATERIAL;
			}
			ofShader* shader = nullptr;
		};

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
		std::vector<IPinInput*> UniformsToPinInputs(ofShader& shader, nodes::INode* node);
	}
};