#pragma once

#include "seam/pins/pin.h"
#include "seam/textureLocationResolver.h"

namespace seam {
    struct SeamState {
        pins::PushPatterns* pushPatterns = nullptr;
        TextureLocationResolver* texLocResolver = nullptr;
    };
}