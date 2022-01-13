#pragma once

#include "pin.h"
#include "../hash.h"

namespace seam::pins {
	using PushFunc = std::function<void(
		char* src, 
		size_t src_size_bytes, 
		char* dst, 
		size_t dst_size_bytes
	)>;

	/// Describes a pattern or strategy for pushing values from an output pin to an input pin.
	/// Helps you propagate changes to input pins, 
	/// but you don't have to use them if you have another way to push changes.
	struct Pusher {
		Pusher(std::string_view _name, PushFunc _func)
			: name(_name), func(_func) 
		{
			id = SCHash(name.data(), name.length());
		}

		PushFunc func;
		std::string_view name;
		/// is hashed from the name
		PushId id;

		bool operator<(const Pusher& other) {
			return id < other.id;
		}

		bool operator==(const Pusher& other) {
			return id == other.id;
		}
	};
	
	class PushPatterns {
	public:
		PushPatterns();

		/// Register a Pusher. If another Pusher with the same name already exists, it will be overridden.
		/// \return true if another Pusher was overridden
		bool Register(Pusher&& pusher);
		
		/// Push data from output pins to their attached input pins.
		/// \param pin the output pin to push data from
		/// \param data_array an array of values to push to the output pin's attached input pins
		/// \param array_len the number of elements in the data array
		template <typename T>
		void Push(const PinOutput& pin, T* data_array, size_t array_len);
		
	private:
		// push patterns are sorted by pusher id
		std::vector<Pusher> push_patterns;
	};
}
