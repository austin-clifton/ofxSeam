#pragma once

#include "stdint.h"

namespace seam {
	// hash function for an array of char
	// thank you supercollider plugin source and Bob Jenkins for le hash function
	// a reference page for the hash is here: https://burtleburtle.net/bob/hash/doobs.html
	// or run a search for "bob jenkins one-at-a-time hash"
	inline uint32_t SCHash(const char* inKey, const size_t len) {
		// the one-at-a-time hash.
		// a very good hash function. ref: a web page by Bob Jenkins.
		uint32_t hash = 0;
		for (size_t i = 0; i < len; ++i) {
			hash += *inKey++;
			hash += (hash << 10);
			hash ^= (hash >> 6);
		}
		hash += (hash << 3);
		hash ^= (hash >> 11);
		hash += (hash << 15);

		return hash;
	}
}