#pragma once

#include <vector>

namespace seam {
	/// a very simple allocation pool, which can be "cleared" and reset
	/// good for per-frame variable-size data, like notes data
	class FramePool {
	public:
		FramePool(size_t size_in_bytes) {
			pool.resize(size_in_bytes);
			pos = 0;
		}

		void Clear() {
			pos = 0;
		}

		template <typename T>
		T* Alloc() {
			// resize if the pool is too small
			if (pos + sizeof(T) > pool.size()) {
				printf("pool is out of space, doubling buffer size to %zu bytes", pool.size() * 2);
				pool.resize(pool.size() * 2);
			}
			// calculate the pointer position of the "allocation"
			T* ptr = (T*)(pool.data() + pos);
			// run the default constructor for the given type 
			*ptr = T();
			// increment position for next "alloc"
			pos += sizeof(T);
			return ptr;
		}

	private:
		std::vector<char> pool;
		size_t pos;
	};
}
