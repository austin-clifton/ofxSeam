#pragma once

#include <cstdint>

namespace seam::props {
    enum class NodePropertyType : uint8_t {
        None,
		Bool,
		Char,
		Float,
		Int,
		Uint,
		String,
		Struct,
	}; 
}