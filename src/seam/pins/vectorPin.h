#pragma once

#include "pinInput.h"

namespace seam::pins {
    class VectorPinInput {
    public:
        using CreateCallback = std::function<PinInput(VectorPinInput*, size_t)>;
        using SetPinPointersCallback = std::function<void(void* ptr, PinInput* pinIn)>;

        struct Options {
            std::function<void(VectorPinInput*)> onSizeChanged;
            std::function<void(VectorPinInput*, size_t)> onPinRemoved;
            std::function<void(PinInput*, size_t)> onPinChanged;
        };

        VectorPinInput(PinType childPinType);

        VectorPinInput(
            CreateCallback&& createChildPin, 
            SetPinPointersCallback&& setPinPointers, 
            size_t elementSize
        );

        PinInput SetupVectorPin(
            nodes::INode* node, 
            PinType pinType, 
            const std::string_view name,
            size_t initialSize = 0
        );

        void SetCallbackOptions(Options&& _options);

        void UpdateSize(size_t newSize);

        inline size_t Size() {
            return buff.size() / elementSize;
        }

        template <typename T>
        T* Get(size_t &size) {
            assert(sizeof(T) == elementSize);
            size = Size();
            return (T*)buff.data();
        }

    private:
        PinInput* vectorPin = nullptr;

        size_t elementSize = 0;
        CreateCallback createCb;
        SetPinPointersCallback fixPointersCb;

        std::vector<char> buff;

        Options options;
    };
}
