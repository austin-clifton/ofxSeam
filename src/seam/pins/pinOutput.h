#pragma once

#include <vector>

#include "seam/pins/pinBase.h"
#include "seam/pins/pinConnection.h"
#include "seam/pins/iOutPinnable.h"

namespace seam::pins {
    struct PinOutput final : public Pin, public IOutPinnable {
        ~PinOutput();

        PinOutput* PinOutputs(size_t& size) {
            size = childPins.size();
            return childPins.data();
        }

        void SetChildren(std::vector<PinOutput>&& children) {
            childPins = std::move(children);
        }

        /// @brief If internals of Pin Output data changes but a push isn't needed 
        /// (for instance, an FBO's contents updated), you can skip push patterns and just dirty connections.
        void DirtyConnections();

        inline uint16_t NumCoords() { return numCoords; }
        void SetNumCoords(uint16_t newNumCoords);

        std::vector<seam::pins::PinConnection> connections;
        std::vector<PinOutput> childPins;

    private:
        uint16_t numCoords = 1;
    };
}