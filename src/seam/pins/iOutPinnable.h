#pragma once

#include <cstdint>
#include "seam/pins/pinBase.h"

namespace seam::pins {
    class PinOutput;

    class IOutPinnable {
    public:
        /// \param size should be set to the size (in elements) of the returned array
        /// \return a pointer to the array of pin outputs
        virtual PinOutput* PinOutputs(size_t& size) = 0;

        static PinOutput* FindPinOut(IOutPinnable* pinnable, PinId id);
    };
}