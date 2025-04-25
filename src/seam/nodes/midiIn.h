#pragma once

// seam's MIDI input implementation uses ofxMIDI:
// https://github.com/danomatika/ofxMidi
#include "ofxMidi.h"

#include "seam/include.h"
#include "seam/containers/ringBuffer.h"

using namespace seam::pins;

namespace seam::nodes {
	class MidiIn : public IDynamicPinsNode, public ofxMidiListener {
	public:
		MidiIn();
		~MidiIn();

		void Setup(SetupParams* params) override;

		void Update(UpdateParams* params) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		/// callback for ofxMidiIn and required method implementation from ofxMidiListener
		void newMidiMessage(ofxMidiMessage& msg) override;

		pins::PinInput* AddPinIn(PinInArgs args) override;
		pins::PinOutput* AddPinOut(pins::PinOutput&& pinOut, size_t index) override;

		// TODO remove and/or clear individual note pins added by AddNotePin()
	private:
		/// Add a Pin that listens to a specific note.
		/// Useful if you want to isolate the kick drum MIDI note, for instance.
		/// For MIDI 1.0, only 0..127 really matter.
		/// \return true if the note was added;
		/// if a note pin for the given midi_note already exists,
		/// the pin will not be added, and false will be returned.
		PinOutput* AddNotePin(uint32_t midi_note);

		/// Listen to the given MIDI port.
		/// return true if the port was successfully opened
		bool ListenOnPort(unsigned int port);

		static bool CompareNotePin(const PinOutput& pin_out, const uint32_t note);

		notes::NoteOnEvent* MidiToNoteOnEvent(ofxMidiMessage& msg, FramePool* alloc_pool);
		notes::NoteOffEvent* MidiToNoteOffEvent(ofxMidiMessage& msg, FramePool* alloc_pool);
		void AttemptPushToNotePin(UpdateParams* params, notes::NoteEvent* ev, int pitch);

		// this node has a variable number of output pins;
		// there is always at least one, an EVENT_QUEUE Pin which pushes ALL note events,
		// plus 0 to many Pins which only send one MIDI note value
		std::vector<PinOutput> pin_outputs = {
			pins::SetupOutputPin(this, pins::PinType::NoteEvent, "all notes stream", 1, pins::PinFlags::EventQueue),
			pins::SetupOutputPin(this, pins::PinType::NoteEvent, "notes on stream", 1, pins::PinFlags::EventQueue),
			pins::SetupOutputPin(this, pins::PinType::NoteEvent, "notes off stream", 1, pins::PinFlags::EventQueue),
		};
		// tracks where user-defined (per-note) Pins begin
		size_t custom_pins_index;

		// dictates which note event types a MIDI input node will push to its children;
		// filter events you don't want out at this layer for better efficiency
		notes::EventTypes listening_event_types = notes::EventTypes(
			(uint8_t)notes::EventTypes::On
			| (uint8_t)notes::EventTypes::Off
		);

		// receives MIDI messages from external sources
		ofxMidiIn midi_in;
		unsigned int midi_port = 0;

		// these are used only for GUI state tracking
		int gui_midi_port = 0;
		int gui_note_add = 0;

		// pushes MIDI messages from midi_in; the update loop drains messages
		// if you manage to make more than 16 MIDI notes in a single frame, wow
		RingBuffer<ofxMidiMessage> messages = RingBuffer<ofxMidiMessage>(16);
	};
}