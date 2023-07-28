#include "notes-printer.h"
#include "seam/containers/octree.h"

using namespace seam;
using namespace seam::nodes;

NotesPrinter::NotesPrinter() : INode("Notes Printer") {
	// this is a debug tool which will never be part of a visual chain (it has no outputs).
	// check for Update() every frame.
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_EVERY_FRAME);
	pinNotesOnStream = &pin_inputs[0];
	pinNotesOnStream = &pin_inputs[1];
}

NotesPrinter::~NotesPrinter() {

	// no op
}

pins::PinInput* NotesPrinter::PinInputs(size_t& size) {
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
	size_t size;
	notes::NoteOnEvent** onEvents = (notes::NoteOnEvent**)pinNotesOnStream->GetEvents(size);

	// drain any notes that were pushed to each notes stream and print their values
	for (size_t i = 0; i < size; i++) {
		PrintNoteOnEvent(onEvents[i]);
	}
	pinNotesOnStream->ClearEvents(size);

	notes::NoteOffEvent** offEvents = (notes::NoteOffEvent**)pinNotesOffStream->GetEvents(size);

	for (size_t i = 0; i < size; i++) {
		PrintNoteOffEvent(offEvents[i]);
	}
	pinNotesOffStream->ClearEvents(size);
}