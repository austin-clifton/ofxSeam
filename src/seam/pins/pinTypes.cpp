#include "seam/pins/pinTypes.h"

namespace seam::pins {
    bool IsFboPin(PinType pinType) {
        return pinType >= PinType::FBO_RGBA && pinType <= PinType::FBO_RED;
    }
}