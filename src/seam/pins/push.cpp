#if RUN_DOCTEST
#include "doctest.h"
#endif

#include "push.h"

using namespace seam::pins;

PushPatterns::PushPatterns() {
	// register default pushers

	Register(Pusher("one to one", [](char* src, size_t src_size, char* dst, size_t dst_size, size_t el_size) 
	{
		// one-to-one means only copy once with no repeats
		// we should simply copy a maximum of all the elements in the src array, 
		// OR up to as many as can fit in the destination
		size_t bytes_to_copy = min(src_size, dst_size);
		std::copy(src, src + bytes_to_copy, dst);
	}));

	Register(Pusher("repeat", [](char* src, size_t src_size, char* dst, size_t dst_size, size_t el_size) 
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

	Register(Pusher("reverse repeat", [](char* src, size_t src_size, char* dst, size_t dst_size, size_t el_size) 
	{
		// like repeat, except in reverse
		// src [1, 2, 3] should end up in dst with size 5 as [3, 2, 1, 3, 2]
		char* dst_off = dst;
		while (dst_off != dst + dst_size) {
			char* src_off = src + src_size;
			while (src_off != src && dst_off != dst + dst_size) {
				src_off -= el_size;
				assert(src_off >= src);	// make sure it doesn't run off
				std::copy(src_off, src_off + el_size, dst_off);
				dst_off += el_size;
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

Pusher& PushPatterns::Get(PushId push_id) {
	auto it = std::lower_bound(push_patterns.begin(), push_patterns.end(), push_id);
	if (it != push_patterns.end() && it->id == push_id) {
		return *it;
	} else {
		// asked for a push id that doesn't exist
		assert(false);
		return *push_patterns.begin();
	}
}

Pusher& PushPatterns::Get(std::string_view name) {
	return Get(SCHash(name.data(), name.length()));
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

#if RUN_DOCTEST
namespace {
	template <typename T>
	void TestPushPattern(const std::vector<T>& input, const std::vector<T>& expected, Pusher& pusher) {
		// allocate a block to store the output data,
		// PLUS some extra space for testing that there are no writes happening past the edges of the write space
		const size_t data_len = expected.size() * sizeof(T);
		char* output = new char[data_len + 2 * sizeof(uint32_t)];
		const uint32_t GUARD = 0xDEADDEAD;
		// write the guard to the very beginning of the block
		*((uint32_t*)output) = GUARD;
		// and the very end
		const uint32_t end_guard_pos = sizeof(uint32_t) + data_len;
		*((uint32_t*)(output + end_guard_pos)) = GUARD;
		
		// now run the push pattern, starting past the beginning guard on the output
		pusher.func(
			(char*)input.data(), 
			input.size() * sizeof(T), 
			output + sizeof(uint32_t), 
			expected.size() * sizeof(T), 
			sizeof(T)
		);

		// make sure no guards were overwritten
		CHECK(*((uint32_t*)(output)) == GUARD);
		CHECK(*((uint32_t*)(output + end_guard_pos)) == GUARD);

		// make sure the output data matches the expected output data
		for (size_t i = 0; i < expected.size(); i++) {
			// this should work for floats since it's a direct copy..?
			size_t offset = sizeof(uint32_t) + sizeof(T) * i;
			CHECK(*((T*)(output + offset)) == expected[i]);
		}
	}
}

TEST_CASE("Testing PushPatterns behaviors") {
	PushPatterns push_patterns;

	// one to one tests
	{
		Pusher& pusher = push_patterns.Get("one to one");
		TestPushPattern<int>({}, {}, pusher);
		TestPushPattern<double>({ 0.3, 0.59, 0.1 }, { 0.3, 0.59, 0.1 }, pusher);
		TestPushPattern<int>({ 3 }, { 3 }, pusher);
		TestPushPattern<int>({ 3, 5 }, { 3, 5 }, pusher);
	}

	// repeat tests
	{
		Pusher& pusher = push_patterns.Get("repeat");
		TestPushPattern<Pusher*>({}, {}, pusher);
		TestPushPattern<char>({ 'h', 'e', 'l', 'l', }, { 'h', 'e', 'l', 'l', 'h', 'e', 'l', 'l', 'h' }, pusher);
		TestPushPattern<int>({ 1, 2, 3 }, { 1 }, pusher);
		TestPushPattern<int>({ 1, 2, 3, 5 }, { 1, 2, 3, 5, 1, 2, 3, 5, 1, 2, 3 }, pusher);
	}

	// reverse repeat tests
	{
		Pusher& pusher = push_patterns.Get("reverse repeat");
		TestPushPattern<int>({}, {}, pusher);
		TestPushPattern<int>({ 101, 2222, 33393 }, { 33393, 2222, 101, 33393, 2222 }, pusher);
		TestPushPattern<char>({ 'h', 'e', 'l', 'l', 'o' }, { 'o', 'l', 'l', 'e', 'h', 'o'}, pusher);
		TestPushPattern<float>({ 0.515, 30.2342 }, { 30.2342 }, pusher);
	}

}
#endif // RUN_DOCTEST