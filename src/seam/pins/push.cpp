#include "push.h"

using namespace seam::pins;

PushPatterns::PushPatterns() {
	// register default pushers

	Register(Pusher("one to one", [](char* src, size_t src_size, char* dst, size_t dst_size) 
	{
		// one-to-one means only copy once with no repeats
		// we should simply copy a maximum of all the elements in the src array, 
		// OR up to as many as can fit in the destination
		size_t bytes_to_copy = min(src_size, dst_size);
		std::copy(src, src + bytes_to_copy, dst);
	}));

	Register(Pusher("repeat", [](char* src, size_t src_size, char* dst, size_t dst_size) 
	{
		// repeat should copy the whole src to the destination,
		// and then repeat starting at 0 again in the src,
		// until dst has been completely filled with repeated data from src.
		size_t dst_off = 0;
		while (dst_off < dst_size) {
			size_t bytes_to_copy = min(src_size, dst_size - dst_off);
			std::copy(src, src + bytes_to_copy, dst + dst_off);
			dst_off += bytes_to_copy;
		}
	}));

	Register(Pusher("reverse repeat", [](char* src, size_t src_size, char* dst, size_t dst_size) 
	{
		// like repeat, except in reverse
		// src [1, 2, 3] should end up in dst with size 5 as [3, 2, 1, 3, 2]
		char* dst_off = dst;
		while (dst_off != dst + dst_size) {
			char* src_off = src + src_size;
			while (src_off != src && dst_off != dst + dst_size) {
				src_off--;
				*dst_off = *src_off;
				dst_off++;
			}
		}
	}));
}

bool PushPatterns::Register(Pusher&& pusher) {
	auto it = std::lower_bound(push_patterns.begin(), push_patterns.end(), pusher);
	bool overwrote = it != push_patterns.end() && it->id == pusher.id;
	push_patterns.insert(it, std::move(pusher));
	return overwrote;
}

template <typename T>
void PushPatterns::Push(const PinOutput& pin_out, T* data, size_t len) {
	const bool is_event_queue_pin =
		(pin_out.pin.flags & pins::PinFlags::EVENT_QUEUE) == pins::PinFlags::EVENT_QUEUE;
	// event queue pins don't use push patterns, they just push to the input pins' vectors
	if (is_event_queue_pin) {
		
		for (IPinInput* ipin_in : pin_out.connections) {
			// safe cast and make sure this is the right type...
			PinInput<T, 0>* pin_in = dynamic_cast<PinInput<T, 0>>(ipin_in);
			// ...but explode if it isn't
			assert(pin_in != nullptr);
			// event queue input pins should always be event queue pins themselves
			assert((pin_in->flags & pins::PinFlags::EVENT_QUEUE) == pins::PinFlags::EVENT_QUEUE);
			// push the output pin's data to the back of the input pin's vector
			pin_in->channels.insert(pin_in->channels.end(), data);
		}
	} else {
		for (IPinInput* pin_in : pin_out.connections) {
			// find the push pattern, and expect it to exist
			auto pp = std::lower_bound(push_patterns.begin(), push_patterns.end(), pin_in->push_id);
			assert(pp->id == pin_in->push_id);
			// grab the destination data
			size_t dst_size;
			char* dst = pin_in->GetChannels(dst_size);
			// call the push pattern 
			pp->func((char*)data, len * sizeof(T), dst, dst_size * sizeof(T));
		}
	}

}

