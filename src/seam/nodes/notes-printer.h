#pragma once

#include "i-node.h"
#include "../pins/pin-note-event.h"

using namespace seam::pins;

namespace seam::nodes {
	/// A little debug utility for printing note events.
	/// Inherit this class and override PrintNoteEvent() to print extended note info.
	class NotesPrinter : public INode {
	public:
		NotesPrinter();
		virtual ~NotesPrinter();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	protected:
		virtual void PrintNoteOnEvent(notes::NoteOnEvent* ev);
		virtual void PrintNoteOffEvent(notes::NoteOffEvent* ev);

	private:
		PinInput* pinNotesOnStream;
		PinInput* pinNotesOffStream;

		std::array<PinInput, 2> pin_inputs = {
			pins::SetupInputQueuePin(PinType::NOTE_EVENT, this, "Notes On Stream"),
			pins::SetupInputQueuePin(PinType::NOTE_EVENT, this, "Notes Off Stream"),
		};


		/*
		PinNoteEvent<0> pin_notes_on_stream = PinNoteEvent<0>(
			"notes on stream",
			"incoming stream of note ON events",
			notes::EventTypes::ON
		);

		PinNoteEvent<0> pin_notes_off_stream = PinNoteEvent<0>(
			"notes off stream",
			"incoming stream of note OFF events",
			notes::EventTypes::OFF
		);

		std::array<PinInput, 2> pin_inputs = {
			&pin_notes_on_stream,
			&pin_notes_off_stream
		};
		*/
	};
}