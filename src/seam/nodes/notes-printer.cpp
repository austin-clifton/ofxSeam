#include "notes-printer.h"

using namespace seam;
using namespace seam::nodes;

NotesPrinter::NotesPrinter() : INode("Notes Printer") {
	// this is a debug tool which will never be part of a visual chain (it has no outputs).
	// check for Update() every frame.
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_EVERY_FRAME);
}

NotesPrinter::~NotesPrinter() {
	// no op
}

pins::IPinInput** NotesPrinter::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

pins::PinOutput* NotesPrinter::PinOutputs(size_t& size) {
	size = 0;
	return nullptr;
}

void NotesPrinter::PrintNoteOnEvent(notes::NoteOnEvent* ev) {
	std::cout << "ON:  id " << ev->instance_id
		<< std::fixed << std::setprecision(2)
		<< "\tvel " << ev->velocity
		<< "\tfreq " << ev->frequency
		<< "\n";
}

void NotesPrinter::PrintNoteOffEvent(notes::NoteOffEvent* ev) {
	std::cout << "OFF: id " << ev->instance_id << "\n";
}

void NotesPrinter::Update(UpdateParams* params) {
	// drain any notes that were pushed to each notes stream and print their values
	for (size_t i = pin_notes_on_stream.channels.size(); i > 0; i--) {
		PrintNoteOnEvent((notes::NoteOnEvent*)pin_notes_on_stream[i - 1]);
		pin_notes_on_stream.channels.pop_back();
	}

	for (size_t i = pin_notes_off_stream.channels.size(); i > 0; i--) {
		PrintNoteOffEvent((notes::NoteOffEvent*)pin_notes_off_stream[i - 1]);
		pin_notes_off_stream.channels.pop_back();
	}
}