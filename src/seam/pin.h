#pragma once

#include <vector>
#include <functional>

#include "ofMain.h"

#include "notes.h"

namespace seam {

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

	// a bitmask enum for marking boolean properties of a Pin
	enum PinFlags {
		IS_FEEDBACK = 1 << 0,
	};

	struct Texture {
		glm::ivec2 resolution;
		ofFbo* fbo = nullptr;
	};

	// base struct for pin types
	// input pins also contain an eventing callback
	// output pins contain their list of connections
	struct Pin {
		PinType type;
		std::string_view name;
		PinFlags flags = (PinFlags)0;
	};

	// TODO pin allocs should maybe come from a pool

	struct PinFlow : Pin {
		PinFlow() {
			type = PinType::FLOW;
		}
		std::function<void()> callback;
	};

	struct PinBool : Pin {
		PinBool() {
			type = PinType::BOOL;
		}
		std::function<void(bool v)> callback;
	};

	struct PinFloat : Pin {
		PinFloat() {
			type = PinType::FLOAT;
		}
		std::function<void(float v)> callback;
	};

	struct PinInt : Pin {
		PinInt() {
			type = PinType::INT;
		}
		std::function<void(int v)> callback;
	};

	struct PinTexture : Pin {
		PinTexture() {
			type = PinType::TEXTURE;
		}
		std::function<void(Texture tex)> callback;
	};

	struct PinNoteOn : Pin {
		PinNoteOn() {
			type = PinType::NOTE_ON;
		}
		std::function<void(BasicNote v)> callback;
	};


	struct PinOutput {
		// output pin doesn't need an event, just need to know the name and type
		// use the base struct
		Pin pin;
		std::vector<Pin*> connections;
	};

	struct PinInput {
		Pin* pin = nullptr;
		Pin* connection = nullptr;
	};

	// template utility function for creating input pins 
	// explicit template speclializations are defined in the cpp file
	// each overload takes a different callback, and infers the pin's type
	// TODO not sure why the template is needed, vc++17 compiler says it can't disambiguate overloads without the template
	template <typename... T>
	PinInput CreatePinInput(const std::string_view name, std::function<void(T...)>&& callback);

};