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

		// flow pins are stateless triggers (no arguments in the event callback)
		// the output pin calls whatever function it's attached to when some criteria is met
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

	// base struct for pin types
	struct Pin {
		PinType type;
		// human-readable Pin name for display purposes
		std::string_view name;
		// human-readable Pin description for display purposes
		std::string_view description;
		PinFlags flags = (PinFlags)0;
	};

	// TODO pin allocs should maybe come from a pool

	struct PinFlow : Pin {
		PinFlow() {
			type = PinType::FLOW;
		}
		// flow pins have no value, the node just needs to be dirtied and updated
	};

	struct PinBool : Pin {
		PinBool() {
			type = PinType::BOOL;
		}
		bool value = false;
	};

	struct PinFloat : Pin {
		PinFloat(
			std::string_view _name = "float",
			std::string_view _description = "",
			float _init_val = 0.f,
			float _min = -FLT_MAX,
			float _max = FLT_MAX,
			RangeType _range_type = RangeType::LINEAR,
			PinFlags _flags = PinFlags::INPUT
		) {
			name = _name;
			description = _description;
			type = PinType::FLOAT;
			flags = _flags;
			value = std::min(_max, std::max(_init_val, _min));
			min = _min;
			max = _max;
			range_type = _range_type;
		}

		float value = 0.f;
		float min = -FLT_MAX;
		float max = FLT_MAX;
		RangeType range_type = RangeType::LINEAR;
	};

	struct PinInt : Pin {
		PinInt(
			std::string_view _name = "int",
			std::string_view _description = "",
			int _init_val = 0,
			int _min = INT_MIN,
			int _max = INT_MAX,
			RangeType _range_type = RangeType::LINEAR,
			PinFlags _flags = PinFlags::INPUT
		) {
			name = _name;
			description = _description;
			type = PinType::INT;
			flags = _flags;
			value = std::min(_max, std::max(_init_val, _min));
			min = _min;
			max = _max;
			range_type = _range_type;
		}

		int value = 0;
		int min = INT_MIN;
		int max = INT_MAX;
		RangeType range_type = RangeType::LINEAR;
	};

	struct PinTexture : Pin {
		PinTexture() {
			type = PinType::TEXTURE;
		}
		Texture value;
	};

	struct PinNoteOn : Pin {
		PinNoteOn() {
			type = PinType::NOTE_ON;
		}
	};

	struct PinInput {
		Pin* pin = nullptr;
		Pin* connection = nullptr;
		IEventNode* node = nullptr;

		bool operator==(const PinInput& other) {
			return pin == other.pin;
		}
	};

	struct PinOutput {
		// output pins don't have values, just need to know metadata (name and type)
		// use the base struct
		Pin pin;
		std::vector<PinInput> connections;
	};

	// TODO this is bad nomenclature

	PinInput SetupPinInput(Pin* pin, IEventNode* node);

	Pin SetupPinOutput(PinType type, std::string_view name, PinFlags flags = (PinFlags)0);
};