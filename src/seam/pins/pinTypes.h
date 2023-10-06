#pragma once

#include <cstdint>

namespace seam::pins {
        enum PinType : uint8_t {
        // invalid pin type
        // used internally but should not be used for actual pins
        TYPE_NONE,

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

        // WTF AUSTIN WHY ARE TEXTURE AND MATERIAL DIFFERENT?????

        // FBO + resolution
        FBO,

        // a material is really just a shader with specific uniform value settings
        MATERIAL,

        // note events send a pointer to a struct from notes.h, 
        // or a struct that inherits one of those structs
        NOTE_EVENT,
    };
}