#pragma once

#include "seam/include.h"

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
	};
}