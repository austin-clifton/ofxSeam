#pragma once

#include "pin.h"

namespace seam::pins {
	using PushFunc = std::function<void(void* src, size_t src_size, void* dst, size_t dst_size, size_t element_size)>;

	/// Describes a pattern or strategy for pushing values from an output pin to an input pin.
	/// Helps you propagate changes to input pins, 
	/// but you don't have to use them if you have another way to push changes.
	struct Pusher {
		PushFunc func;
		std::string_view name;
		/// is hashed from the name
		PushId id;
	};
	
	class PushPatterns {
	public:
		template <typename T>
		void Push(const PinOutput& pin, T* data);

		
	};
}
