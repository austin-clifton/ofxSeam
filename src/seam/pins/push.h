#pragma once

#include "pin.h"
#include "pin-int.h"
#include "pin-flow.h"
#include "pin-float.h"
#include "../hash.h"
#include "../flags-helper.h"

namespace seam::pins {
	using PushFunc = std::function<void(
		char* src, 
		size_t src_size_bytes, 
		char* dst, 
		size_t dst_size_bytes,
		size_t element_size
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

		bool operator<(const PushId other_id) {
			return id < other_id;
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
		void Push(const PinOutput& pin_out, T* data, size_t len) {
			const bool is_event_queue_pin = flags::AreRaised(
				pin_out.pin.flags, 
				pins::PinFlags::EVENT_QUEUE
			);

			// event queue pins don't use push patterns, they just push to the input pins' vectors
			if (is_event_queue_pin) {
				for (IPinInput* ipin_in : pin_out.connections) {
					PinInput<T, 0>* pin_in = (PinInput<T, 0>*)ipin_in;
					// dirty the input node
					pin_in->node->SetDirty();
					// event queue input pins should always be event queue pins themselves
					assert((pin_in->flags & pins::PinFlags::EVENT_QUEUE) == pins::PinFlags::EVENT_QUEUE);
					// push the output pin's data to the back of the input pin's vector
					pin_in->channels.insert(pin_in->channels.end(), data, data + len);
				}
			} else {
				for (IPinInput* pin_in : pin_out.connections) {
					// find the push pattern, and expect it to exist
					auto pp = std::lower_bound(push_patterns.begin(), push_patterns.end(), pin_in->push_id);
					assert(pp->id == pin_in->push_id);
					// grab the destination data
					size_t dst_size;
					char* dst = (char*)pin_in->GetChannels(dst_size);
					// dirty the input node
					pin_in->node->SetDirty();
					// call the push pattern 
					pp->func((char*)data, len * sizeof(T), dst, dst_size * sizeof(T), sizeof(T));
				}
			}
		}

		template <size_t N>
		void Push(const PinOutput& pin_out, PinFloat<N>* pin_in) {
			// TODO!
			assert(false);
		}

		template <size_t N>
		void Push(const PinOutput& pin_out, PinInt<N>* pin_in) {
			// TODO!
			assert(false);
		}

		void Push(const PinOutput& pin_out, PinFlow* pin_in) {
			assert(pin_out.pin.type == PinType::FLOW && pin_in->type == PinType::FLOW);
			pin_in->Callback();
		}

		Pusher& Get(PushId push_id);
		Pusher& Get(std::string_view name);

		Pusher& Default();
		void SetDefault(PushId push_id);
		void SetDefault(std::string_view name);
		
	private:
		// push patterns are sorted by pusher id
		std::vector<Pusher> push_patterns;

		Pusher* default_pattern = nullptr;
	};
}
