#pragma once

#include "pinInput.h"
#include "pinOutput.h"

namespace {
    template <typename SrcT, typename DstT>
    void Convert(void* src, void* dst) {
        *(DstT*)dst = *(SrcT*)src;
    }
}

namespace seam::pins {
    using ConvertSingle = std::function<void(void* src, void* dst)>;
    using ConvertMulti = std::function<void(void* src, size_t srcSize, void* dst, size_t dstSize)>;

    struct PinConnection {
        PinConnection(PinInput* _input, PinType outputType);
        void RecacheConverts(PinType outputType);

        PinInput* input;
        PinId inputPinId;
        ConvertSingle convertSingle;
        ConvertMulti convertMulti;
    };

    
    ConvertSingle GetConvertSingle(PinType srcType, PinType dstType, bool& isConvertible);
    ConvertMulti GetConvertMulti(PinType srcType, PinType dstType, bool& isConvertible);
}