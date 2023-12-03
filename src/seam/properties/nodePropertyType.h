#pragma once

#include <cstdint>

namespace seam::props {
    enum NodePropertyType : uint8_t {
        PROP_NONE,
		PROP_BOOL,
		PROP_CHAR,
		PROP_FLOAT,
		PROP_INT,
		PROP_UINT,
		PROP_STRING,
		PROP_STRUCT,
	}; 
}