#pragma once

#include <vector>

#include "pinBase.h"
#include "seam/pins/iOutPinnable.h"

namespace seam::pins {
    class PinConnection;

    struct PinOutput final : public Pin, public IOutPinnable {
        std::vector<seam::pins::PinConnection> connections;

        // user data, if any is needed
        void* userp = nullptr;

        PinOutput* PinOutputs(size_t& size) {
            size = childrenSize;
            return childOutputs;
        }

        void SetChildren(PinOutput* children, size_t size) {
            childOutputs = children;
            childrenSize = size;
        }

    private:
        PinOutput* childOutputs = nullptr;
        size_t childrenSize = 0;

    };
}