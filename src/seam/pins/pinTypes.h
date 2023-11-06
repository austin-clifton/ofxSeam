#pragma once

#include <cstdint>

namespace seam::pins {
        enum PinType : uint16_t {
        // invalid pin type
        // used internally but should not be used for actual pins
        TYPE_NONE = 0,

        /// @brief Only valid for input pins
        ANY,

        // flow pins are stateless triggers which just dirty any nodes that use them as inputs
        FLOW,

        // basic types
        // the output pins for these are triggers with arguments
        BOOL,
        CHAR,
        INT,
        UINT,
        FLOAT,
        STRING,
        
        // FBO + resolution
        FBO,

        // note events send a pointer to a struct from notes.h, 
        // or a struct that inherits one of those structs
        NOTE_EVENT,
    };
}