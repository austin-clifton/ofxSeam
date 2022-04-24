#pragma once

#include "pin.h"
#include "../notes.h"

namespace seam::pins {
	struct PinNoteEventMeta {
		/// specifies what note event types this input pin accepts
		notes::EventTypes note_types;
	};

	template <std::size_t N>
	struct PinNoteEvent : public PinInput<notes::NoteEvent*, N>, public PinNoteEventMeta {
		PinNoteEvent(
			std::string_view _name = "note event",
			std::string_view _description = "",
			notes::EventTypes _note_types = notes::EventTypes::ON,
			PinFlags _flags = PinFlags::INPUT
		)
			: PinInput<notes::NoteEvent*, N>({})
		{
			name = _name;
			description = _description;
			type = PinType::NOTE_EVENT;
			note_types = _note_types;
			flags = PinFlags(flags | _flags);
		}
	};
}