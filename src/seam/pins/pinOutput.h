#pragma once

#include <vector>

#include "pinBase.h"

namespace seam::pins {
    class PinConnection;

    // Why doesn't this just inherit Pin...?
    struct PinOutput {
        // output pins don't have values, just need to know metadata (name and type)
        // use the base struct
        Pin pin;
        std::vector<seam::pins::PinConnection> connections;
        // user data, if any is needed
        void* userp = nullptr;
    };
}