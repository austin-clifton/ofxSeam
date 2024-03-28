#pragma once

namespace seam {
    class TextureLocationResolver;
}

namespace seam::pins {
    class PushPatterns;
}

namespace seam {
    struct SeamState {
        seam::pins::PushPatterns* pushPatterns = nullptr;
        seam::TextureLocationResolver* texLocResolver = nullptr;
    };
}