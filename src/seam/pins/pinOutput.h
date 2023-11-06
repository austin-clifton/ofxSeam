#pragma once

#include <vector>

#include "pinBase.h"
#include "seam/pins/iOutPinnable.h"

namespace seam::pins {
    class PinConnection;

    struct PinOutput final : public Pin, public IOutPinnable {
        std::vector<seam::pins::PinConnection> connections;

        PinOutput* PinOutputs(size_t& size) {
            size = childrenSize;
            return childOutputs;
        }

        void SetChildren(PinOutput* children, size_t size) {
            childOutputs = children;
            childrenSize = size;
        }

        /// @brief If internals of Pin Output data changes but a push isn't needed 
        /// (for instance, an FBO's contents updated), you can skip push patterns and just dirty connections.
        void DirtyConnections();

    private:
        PinOutput* childOutputs = nullptr;
        size_t childrenSize = 0;

    };
}