#pragma once

#include <functional>
#include "seam/pins/pinBase.h"

namespace seam::pins {
    class PinInput;
    class PinOutput;

    struct ConvertSingleArgs {
        ConvertSingleArgs(void* _src, uint16_t _srcNumCoords, 
            void* _dst, uint16_t _dstNumCoords,
            PinInput* _pinIn) {
            src = _src;
            srcNumCoords = _srcNumCoords;
            dst = _dst;
            dstNumCoords = _dstNumCoords;
            pinIn = _pinIn;
        }

        void* src;
        uint16_t srcNumCoords;
        void* dst;
        uint16_t dstNumCoords;
        PinInput* pinIn;
    };

    struct ConvertMultiArgs {
        ConvertMultiArgs(void* _src, uint16_t _srcNumCoords, size_t _srcSize, PinInput* _pinIn) {
            src = _src;
            srcNumCoords = _srcNumCoords;
            srcSize = _srcSize;
            pinIn = _pinIn;
        }

        void* src;
        uint16_t srcNumCoords;
        size_t srcSize;
        PinInput* pinIn;
    };

    using ConvertSingle = std::function<void(ConvertSingleArgs)>;
    using ConvertMulti = std::function<void(ConvertMultiArgs)>;

    struct PinConnection {
        PinConnection(PinInput* _input, PinOutput* _output);
        void RecacheConverts();

        PinId inputPinId;
        PinInput* pinIn;
        PinOutput* pinOut;
        ConvertSingle convertSingle;
        ConvertMulti convertMulti;
    };
    
    ConvertSingle GetConvertSingle(PinType srcType, PinType dstType, bool& isConvertible);
    ConvertMulti GetConvertMulti(PinInput* pinIn, PinOutput* pinOut, bool& isConvertible);
}