#include "seam/pins/pinTypes.h"

namespace seam::pins {
    bool IsFboPin(PinType pinType) {
        return pinType >= PinType::FboRgba && pinType <= PinType::FboRed;
    }
}