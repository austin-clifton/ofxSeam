#pragma once

#include <cstdint>

namespace seam::pins {
        enum class PinType : uint16_t {
        // invalid pin type
        // used internally but should not be used for actual pins
        None = 0,

        /// @brief Only valid for input pins
        Any,

        /// @brief Flow pins are stateless triggers which just dirty any nodes that use them as inputs
        Flow,

        // basic types
        // the output pins for these are triggers with arguments
        Bool,
        Char,
        Int,
        Uint,
        Float,
        String,
        
        /// @brief Note events send a pointer to a struct from notes.h, 
        // or a struct that inherits one of those structs
        NoteEvent,

        // Not assignable, for pins which are really just containers for child pins
        Struct,

        // Start counting FBOs at an offset so there's room for expansion, 
        // since there are different FBO formats,
        // and not all of them should be assignable to each other.
        FboRgba = 1000,
        /// @brief 16 bit floating point buffer FBO, good for HDR!
        FboRgba16F = FboRgba + 1,
        FboRed = FboRgba + 2,
    };

    bool IsFboPin(PinType pinType);
}