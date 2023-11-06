#pragma once

#include <cstdint>
#include "seam/pins/pinBase.h"

namespace seam::pins {
    class PinInput;

    class IInPinnable {
    public:
        /// \param size should be set to the size (in elements) of the returned array
		/// \return a pointer to the array of pointers to pin inputs
		virtual PinInput* PinInputs(size_t& size) = 0;

        static PinInput* FindPinIn(IInPinnable* pinnable, PinId id);

        /// @brief When the buffer for dynamically alloc'd pin inputs changes, this function needs to be called,
		/// so that any Connections can re-cache each pointer to the PinInput
		void RecacheInputConnections();

    private:
		void RecacheInputConnections(PinInput* inputs, size_t size);
    };
}