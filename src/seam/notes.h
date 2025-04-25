#pragma once

namespace seam::notes {
	enum class EventTypes : uint8_t {
		None		= 0,
		On			= 1 << 0,
		Off			= 1 << 1,
		Update		= 1 << 2,
		All			= On | Off | Update,
	};
	
	struct NoteEvent {
		EventTypes type;
		/// each note will have a unique id for its on / updated / off events
		uint32_t instance_id;
	};

	struct NoteOnEvent : public NoteEvent {
		NoteOnEvent() { type = EventTypes::On; }
		/// frequency in hertz, 440.f == MIDI 69 == that one A key
		float frequency;
		/// [0..1] how loud is this note
		float velocity;
	};

	/// this base struct doesn't contain any additional info;
	/// if your note events have update data, inherit this struct to tack on additional data
	struct NoteUpdatedEvent : public NoteEvent {
		NoteUpdatedEvent() { type = EventTypes::Update; }
	};

	/// like NoteUpdatedEvent, the base struct doesn't contain additional info;
	/// if you have extra info to tack on to a note off event, inherit this struct to do so
	struct NoteOffEvent : public NoteEvent {
		NoteOffEvent() { type = EventTypes::Off; }
	};


	// TODO
	// osc
	// vst?
	// supercollider
	// etc.
}