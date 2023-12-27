#pragma once

#include <cstdint>

namespace seam::pins {
        enum PinType : uint16_t {
        // invalid pin type
        // used internally but should not be used for actual pins
        TYPE_NONE = 0,

        /// @brief Only valid for input pins
        ANY,

        /// @brief Flow pins are stateless triggers which just dirty any nodes that use them as inputs
        FLOW,

        // basic types
        // the output pins for these are triggers with arguments
        BOOL,
        CHAR,
        INT,
        UINT,
        FLOAT,
        STRING,
        
        /// @brief Note events send a pointer to a struct from notes.h, 
        // or a struct that inherits one of those structs
        NOTE_EVENT,

        // Not assignable, for pins which are really just containers for child pins
        STRUCT,

        // Start counting FBOs at an offset so there's room for expansion, 
        // since there are different FBO formats,
        // and not all of them should be assignable to each other.
        FBO_RGBA = 1000,
        /// @brief 16 bit floating point buffer FBO, good for HDR!
        FBO_RGBA16F = FBO_RGBA + 1,
        FBO_RED = FBO_RGBA + 2,
    };
}