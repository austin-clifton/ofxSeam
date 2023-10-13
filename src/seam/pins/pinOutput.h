#pragma once

#include <vector>

#include "pinBase.h"

namespace seam::pins {
    class PinConnection;

    // Why doesn't this just inherit Pin...?
    struct PinOutput : public Pin {
        std::vector<seam::pins::PinConnection> connections;

        // user data, if any is needed
        void* userp = nullptr;
    };
}