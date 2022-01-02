#pragma once

#include <vector>

#include "ofMain.h"

#include "notes.h"

namespace seam {

	class IEventNode;

	enum PinType : uint8_t {
		// invalid pin type
		// used internally but should not be used for actual pins
		NONE,

		// flow pins are stateless triggers which just dirty any nodes that use them as inputs
		FLOW,

		// basic types
		// the output pins for these are triggers with arguments
		BOOL,
		CHAR,
		INT,
		FLOAT,
		STRING,

		// FBO + resolution
		TEXTURE,

		// a material is really just a shader with specific uniform value settings
		MATERIAL,

		// Note* events are somewhat special: they use a base BasicNote struct which may be masked as a more complex struct.
		// the BasicNote type contains velocity and pitch, but you can create other note types that inherit BasicNote,
		// and then make other Nodes that are capable of handling the more complex Note type

		// just about all note types should support note on and off
		NOTE_ON,
		NOTE_OFF,
		// MIDI notes don't support this
		// for SC notes and possibly other inputs?
		NOTE_CHANGED,
	};

	// TODO move me
	enum RangeType : uint8_t {
		LINEAR,
		// logarithmic
		LOG,
	};

	// a bitmask enum for marking boolean properties of a Pin
	enum PinFlags : uint16_t {
		INPUT = 1 << 0,
		OUTPUT = 1 << 1,
		// only INPUT pins can be marked as feedback
		FEEDBACK = 1 << 2,

	};

	struct Texture {
		glm::ivec2 resolution;
		ofFbo* fbo = nullptr;
	};

	// base struct for pin types, holds metadata about the pin
	struct Pin {
		PinType type;

		// TODO make name and description string-view-ifiable again somehow

		// human-readable Pin name for display purposes
		std::string name;
		// human-readable Pin description for display purposes
		std::string description;

		// Pins keep track of the node they are associated with;
		// is expected to have a valid pointer value
		IEventNode* node = nullptr;

		PinFlags flags = (PinFlags)0;
	};

	struct PinOutput;

	/// Abstract class for input pins.
	/// Defines a generic function to retrieve the channels array which must be implemented.
	class IPinInput : public Pin {
	public:
		virtual ~IPinInput() { }

		/// Get a pointer to the input pin's raw channels data.
		/// \param size should be set to the size of the returned array
		/// \return the c-style array of channels the input Pin owns
		virtual void* GetChannels(size_t& size) = 0;

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
		PinInput(const std::vector<T>& init_vals = {} ) {
			channels = init_vals;
		}

		virtual ~PinInput() { }

		inline void* GetChannels(size_t& size) override {
			size = channels.size();
			// return the address of the first element in the vector
			return &channels[0];
		}

		inline T& operator[](size_t index) {
			assert(index < channels.size());
			return channels[i];
		}

		std::vector<T> channels;
	};

	// TODO pin allocs should maybe come from a pool

	// flow pins are always a single channel since they hold no state,
	// and are only used to dirty nodes
	struct PinFlow : public PinInput<bool, 0> {
		PinFlow() : PinInput<bool, 0>() {
			type = PinType::FLOW;
		}

		// flow pins have no value, the node just needs to be dirtied and updated

	};

	template <std::size_t N>
	struct PinBool : public PinInput<bool, N> {
		PinBool() {
			type = PinType::BOOL;
		}
		// bool value = false;
	};

	struct PinFloatMeta {
		float min = -FLT_MAX;
		float max = FLT_MAX;
		RangeType range_type = RangeType::LINEAR;
	};

	template <std::size_t N>
	struct PinFloat : public PinInput<float, N>, public PinFloatMeta {
		PinFloat(
			std::string_view _name = "float",
			std::string_view _description = "",
			std::array<float, N> _init_vals = { 0 },
			float _min = -FLT_MAX,
			float _max = FLT_MAX,
			RangeType _range_type = RangeType::LINEAR,
			PinFlags _flags = PinFlags::INPUT
		)
			: PinInput<float, N>(_init_vals)
		{
			name = _name;
			description = _description;
			type = PinType::FLOAT;
			flags = _flags;
			min = _min;
			max = _max;
			range_type = _range_type;
		}
	};

	struct PinIntMeta {
		int min = INT_MIN;
		int max = INT_MAX;
		RangeType range_type = RangeType::LINEAR;
	};

	template <std::size_t N>
	struct PinInt : public PinInput<int, N>, public PinIntMeta {
		PinInt(
			std::string_view _name = "int",
			std::string_view _description = "",
			std::array<int, N> init_vals = { 0 },
			int _min = INT_MIN,
			int _max = INT_MAX,
			RangeType _range_type = RangeType::LINEAR,
			PinFlags _flags = PinFlags::INPUT
		)
			: PinInput<int, N>(init_vals)
		{
			name = _name;
			description = _description;
			type = PinType::INT;
			flags = _flags;
			min = _min;
			max = _max;
			range_type = _range_type;
		}
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

	/*
	struct PinNoteOn : Pin {
		PinNoteOn() {
			type = PinType::NOTE_ON;
		}
	};
	*/

	struct PinOutput {
		// output pins don't have values, just need to know metadata (name and type)
		// use the base struct
		Pin pin;
		std::vector<IPinInput*> connections;
	};

	// TODO this is bad nomenclature (SetupPinOutput doesn't return a PinOutput)
	// probably need to redo the PinInput and PinOutput nomenclature

	Pin SetupOutputPin(IEventNode* node, PinType type, std::string_view name, PinFlags flags = (PinFlags)0);

	/// Queries a linked shader program's active uniforms and creates a PinInput list from them.
	/// \param shader The linked shader program to query the uniforms of.
	/// \param node The node the shader's Pins will be added to. 
	/// \return The list of PinInputs which maps to the uniforms in the shader.
	/// All IPinInput structs are heap-allocated, and must be freed when no longer in use.
	std::vector<IPinInput*> UniformsToPinInputs(ofShader& shader, IEventNode* node);
};