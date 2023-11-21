#pragma once

#include <functional>
#include "seam/pins/pinBase.h"

namespace seam::pins {
    class PinInput;

    using ConvertSingle = std::function<void(void* src, void* dst)>;
    using ConvertMulti = std::function<void(void* src, size_t srcSize, PinInput* pinIn)>;

    struct PinConnection {
        PinConnection(PinInput* _input, PinType outputType);
        void RecacheConverts(PinType outputType);

        PinInput* input;
        PinId inputPinId;
        ConvertSingle convertSingle;
        ConvertMulti convertMulti;
    };
    
    ConvertSingle GetConvertSingle(PinType srcType, PinType dstType, bool& isConvertible);
    ConvertMulti GetConvertMulti(PinType srcType, PinInput* pinIn, bool& isConvertible);
}