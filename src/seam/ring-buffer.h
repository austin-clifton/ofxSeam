#pragma once

/// Circular buffer which acts as a queue with read and write indices.
/// A separate reader (pop) thread and writer (push) thread can run lock-free,
/// as long as there is only one thread each for read and write.
template <typename T>
class RingBuffer {
public:
	RingBuffer(uint32_t _capacity)
		: capacity(_capacity)
	{
		arr = new T[capacity];
	}

	~RingBuffer() {
		delete arr;
	}

	/// push (write) to the tail
	inline void Push(const T val) {
		arr[tail] = val;
		tail = (tail + 1) % capacity;
		size += 1;
		assert(size <= capacity);
	}

	/// pop (read) the front
	inline bool Pop(T& ret) {
		if (size) {
			ret = arr[head];
			head = (head + 1) % capacity;
			size -= 1;
			return true;
		}
		return false;
	}

	inline uint32_t NumAvailable() {
		return size;
	}

private:
	T* arr;
	uint32_t head = 0;
	uint32_t tail = 0;
	uint32_t size = 0;
	const uint32_t capacity = 0;
};