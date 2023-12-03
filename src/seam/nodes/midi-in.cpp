#include "midi-in.h"
#include "../flags-helper.h"
#include "../imgui-utils/properties.h"

using namespace seam::pins;
using namespace seam::nodes;
using namespace seam::notes;

namespace {
	float MidiToFreq(int midi_note) {
		// that one a note is 440 hz == note 69
		// and the scaling is logarithmic
		return 440.f * pow(2.f, (midi_note - 69) / 12.f);
	}
}

MidiIn::MidiIn() : INode("MIDI In") {
	// external input requires updating every frame
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_EVERY_FRAME);
	custom_pins_index = pin_outputs.size();

	ListenOnPort(midi_port);

	midi_in.addListener(this);
	// TODO make this settable
	midi_in.setVerbose(true);
}

MidiIn::~MidiIn() {
	// nothing to do?
}

bool MidiIn::ListenOnPort(unsigned int port) {
	midi_port = port;
	gui_midi_port = port;
	if (midi_in.openPort(midi_port)) {
		printf("MIDI listening on port %u\n", midi_port);
		return true;
	} else {
		printf("failed to open MIDI input on port %u\n", midi_port);
		return false;
	}
}

PinInput* MidiIn::PinInputs(size_t& size) {
	size = 0;
	return nullptr;
}

PinOutput* MidiIn::PinOutputs(size_t& size) {
	size = pin_outputs.size();
	return pin_outputs.data();
}

bool MidiIn::CompareNotePin(const PinOutput& pin_out, const uint32_t note) {
	// userp isn't a pointer, it's actually the MIDI note
	size_t pin_note = (size_t)(pin_out.userp);
	return pin_note < note;
}

bool MidiIn::AddNotePin(uint32_t midi_note) {

	auto it = std::lower_bound(
		pin_outputs.begin() + custom_pins_index,
		pin_outputs.end(),
		midi_note,
		&MidiIn::CompareNotePin
	);

	// make sure the note pin doesn't already exist
	// userp will contain the midi note if it already exists
	if (it != pin_outputs.end() && (size_t)it->userp == (size_t)midi_note) {
		return false;
	} else {
		// TODO this will get rekt if Pin::name goes back to being backed by string_view instead of string
		// really need some way to allow both statically alloc'd and dynamically alloc'd names
		std::string pin_name = "Note " + std::to_string(midi_note);

		PinOutput pin_out = pins::SetupOutputPin(
			this,
			pins::PinType::NOTE_EVENT,
			pin_name,
			1,
			PinFlags::EVENT_QUEUE,
			// wheehoo
			// cast midi_note to a size_t, and then mask it as a void* ;
			// it is treated as a size_t internally
			(void*)((size_t)midi_note)
		);

		pin_outputs.insert(it, std::move(pin_out));

		return true;
	}
}

void MidiIn::newMidiMessage(ofxMidiMessage& msg) {
	// all we do here is push to the ring buffer;
	// the messages will be processed and drained in Update()
	messages.Push(msg);
	SetDirty();
}

bool MidiIn::GuiDrawPropertiesList(UpdateParams* params) {
	// TODO display recently received MIDI messages?

	bool changed = false;

	// allow setting the MIDI port in the GUI
	// cast to an int because I'm lazy and don't wanna use DragScalar()
	if (ImGui::DragInt("MIDI Port", &gui_midi_port, .1f, 0, 127)) {
		// user requested a new port
		// gonna assume it's not greater than 2^31 and do the cast here...
		ListenOnPort((unsigned int)gui_midi_port);
		changed = true;
	}

	// draw the interface which allows users to add pins for individual notes
	bool pressed = ImGui::Button("+");
	ImGui::SameLine();
	ImGui::DragInt("Add Note Pin", &gui_note_add, .1f, 0, 127);
	if (pressed) {
		if (!AddNotePin((uint32_t)gui_note_add)) {
			// TODO handle error case!
			// you should probably write to a log
			// which exposed to the user here
		} else {
			changed = true;
		}
	}

	// draw the interface that allows users to add and remove pins
	// TODO PinFlags::DELETABLE and GUI changes
	// TODO to remove pins, do you need to do it later? or can it be done right here?
	// I'm worried it'll fuck up an iterator somewhere...

	return changed;
}

seam::notes::NoteOnEvent* MidiIn::MidiToNoteOnEvent(ofxMidiMessage& msg, seam::FramePool* alloc_pool) {
	// use the frame pool to get space for the note event
	NoteOnEvent* ev = alloc_pool->Alloc<NoteOnEvent>();
	// convert pitch from MIDI note to frequency in hz
	ev->frequency = MidiToFreq(msg.pitch);
	// scale velocity from MIDI [0..127] to [0..1]
	ev->velocity = msg.velocity / 127.f;
	assert(ev->velocity >= 0.f && ev->velocity <= 1.f);
	// for MIDI notes, the instance ID is just the MIDI note;
	// making the assumption here there only one "note" at a time per key / synth pad / whatever
	ev->instance_id = msg.pitch;
	return ev;
}

seam::notes::NoteOffEvent* MidiIn::MidiToNoteOffEvent(ofxMidiMessage& msg, seam::FramePool* alloc_pool) {
	// frame pool alloc a note off event
	NoteOffEvent* ev = alloc_pool->Alloc<NoteOffEvent>();
	// instance id is the note's MIDI pitch (just like with note on)
	ev->instance_id = msg.pitch;
	return ev;
}

void MidiIn::AttemptPushToNotePin(UpdateParams* params, NoteEvent* ev, int pitch) {
	// check if there's a pin for changes to this specific note
	// first search for it
	auto it = std::lower_bound(
		pin_outputs.begin() + custom_pins_index,
		pin_outputs.end(),
		pitch,
		&MidiIn::CompareNotePin
	);
	// is this a valid pin, and is the pin listening for notes with this pitch?
	if (it != pin_outputs.end() && ((size_t)(it->userp)) == pitch) {
		params->push_patterns->Push(*it, &ev, 1);
	}
}

void MidiIn::Update(UpdateParams* params) {
	// drain the messages queue
	ofxMidiMessage msg;
	while (messages.Pop(msg)) {
		// push each message to the event queue pins,
		// if the message type is one we care about
		if (msg.status == MIDI_NOTE_ON && flags::AreRaised(listening_event_types, EventTypes::ON)) {
			notes::NoteOnEvent* ev = MidiToNoteOnEvent(msg, params->alloc_pool);
			// push to all notes stream and notes on stream
			params->push_patterns->Push(pin_outputs[0], &ev, 1);
			params->push_patterns->Push(pin_outputs[1], &ev, 1);
			AttemptPushToNotePin(params, ev, msg.pitch);

		} else if (msg.status == MIDI_NOTE_OFF && flags::AreRaised(listening_event_types, EventTypes::OFF)) {
			notes::NoteOffEvent* ev = MidiToNoteOffEvent(msg, params->alloc_pool);
			// push to all notes stream and notes off stream
			params->push_patterns->Push(pin_outputs[0], &ev, 1);
			params->push_patterns->Push(pin_outputs[2], &ev, 1);
			AttemptPushToNotePin(params, ev, msg.pitch);
		}
	}
}