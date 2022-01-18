#pragma once

#include "pin.h"
#include "../notes.h"

namespace seam::pins {
	struct PinNoteEventMeta {
		/// specifies what note event types this input pin accepts
		notes::EventType note_types;
	};

	template <std::size_t N>
	struct PinNoteEvent : public PinInput<notes::NoteEvent*, N>, public PinNoteEventMeta {
		PinNoteEvent(
			std::string_view _name = "note event",
			std::string_view _description = "",
			notes::EventType _note_types = notes::EventType::ON,
			PinFlags _flags = PinFlags::INPUT
		)
			: PinInput<notes::NoteEvent*, N>()
		{
			name = _name;
			description = _description;
			note_types = _note_types;
			flags = _flags;
		}
	};
}