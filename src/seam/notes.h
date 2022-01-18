namespace seam::notes {
	enum EventType : uint8_t {
		ON			= 1 << 0,
		OFF			= 1 << 1,
		UPDATE		= 1 << 2,
	};
	
	struct NoteEvent {
		EventType type;
		/// each note will have a unique id for its on / updated / off events
		uint32_t instance_id;
	};

	struct NoteOnEvent : public NoteEvent {
		NoteOnEvent() { type = EventType::ON; }
		/// 440.f == MIDI 69 == that one A key
		float pitch;
		/// [0..1] how loud is this note
		float velocity;
	};

	/// this base struct doesn't contain any additional info;
	/// if your note events have update data, inherit this struct to tack on additional data
	struct NoteUpdatedEvent : public NoteEvent {
		NoteUpdatedEvent() { type = EventType::UPDATE; }
	};

	/// like NoteUpdatedEvent, the base struct doesn't contain additional info;
	/// if you have extra info to tack on to a note off event, inherit this struct to do so
	struct NoteOffEvent : public NoteEvent {
		NoteOffEvent() { type = EventType::OFF; }
	};


	// TODO
	// osc
	// vst?
	// supercollider
	// etc.
}