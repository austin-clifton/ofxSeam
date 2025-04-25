#include "seam/pins/vectorPin.h"
#include "seam/include.h"

using namespace seam::pins;

VectorPinInput::VectorPinInput(PinType childPinType) {
    elementSize = PinTypeToElementSize(childPinType);
    
    createCb = [this, childPinType](VectorPinInput* me, size_t i) -> PinInput {
        assert(vectorPin != nullptr);
        size_t size;
        vectorPin->PinInputs(size);

        return SetupInputPin(childPinType, nullptr,
            nullptr, 1, std::to_string(size));
    };

    fixPointersCb = [this](void* buff, PinInput* pinIn) {
        pinIn->SetBuffer(buff, 1);
    };
}

VectorPinInput::VectorPinInput(
    VectorPinInput::CreateCallback&& _createCb,
    VectorPinInput::SetPinPointersCallback&& _setPinPointersCb,
    size_t _elementSize
) {
    elementSize = _elementSize;
    createCb = _createCb;
    fixPointersCb = _setPinPointersCb;
    assert(!createCb == false);
    assert(!fixPointersCb == false);
}

PinInput VectorPinInput::SetupVectorPin(
    nodes::INode* node, 
    PinType pinType, 
    const std::string_view name,
    size_t initialSize
) {
    assert(elementSize != 0);
    PinInput pinIn = SetupInputPin(pinType, node, nullptr, 0, name);
    pinIn.flags = (PinFlags)(pinIn.flags | PinFlags::Vector);
    pinIn.seamp = this;

    // vectorPin = &pinIn;

    UpdateSize(initialSize);
    return pinIn;
}

void VectorPinInput::SetCallbackOptions(Options&& _options) {
    options = std::move(_options);

    if (vectorPin == nullptr) {
        return;
    }

    size_t size;
    PinInput* childPins = vectorPin->PinInputs(size);

    if (options.onPinChanged) {
        // Make sure all existing pins' callbacks are updated.
        // TODO this doesn't handle hierarchies / structs!
        for (size_t i = 0; i < vectorPin->childPins.size(); i++) {
            PinInput& childPin = vectorPin->childPins[i];
            childPins[i].SetOnValueChanged([this, i]() {
                options.onPinChanged(&vectorPin->childPins[i], i);
            });
        }
    } else {
        // Clear existing callbacks
        for (size_t i = 0; i < size; i++) {
            childPins[i].SetOnValueChanged(std::function<void(void)>());
        }
    }
}

void VectorPinInput::UpdateSize(size_t newSize) {
    if (vectorPin == nullptr) {
        return;
    }
    assert(vectorPin != nullptr);

    if (Size() == newSize) {
        return;
    }

    buff.resize(newSize * elementSize);
    vectorPin->SetBuffer(buff.data(), buff.size() / elementSize);

    // Set up any new additional child pins.
    while (vectorPin->childPins.size() < newSize) {
        std::function<void(void)> changedCb;
        if (options.onPinChanged) {
            size_t pinIndex = vectorPin->childPins.size();
            changedCb = [this, pinIndex]() {
                options.onPinChanged(&vectorPin->childPins[pinIndex], pinIndex);
            };
        }

        PinInput childPin = createCb(this, vectorPin->childPins.size());
        childPin.node = vectorPin->node;
        childPin.SetOnValueChanged(std::move(changedCb));
        vectorPin->childPins.push_back(std::move(childPin));
    }

    // Cut off any pins that don't need to exist anymore.
    if (vectorPin->childPins.size() > newSize) {
        if (options.onPinRemoved) {
            while (vectorPin->childPins.size() > newSize) {
                options.onPinRemoved(this, vectorPin->childPins.size() - 1);
                vectorPin->childPins.pop_back();
            }
        } else {
            vectorPin->childPins.resize(newSize);
        }
    }

    assert(buff.size() / elementSize == vectorPin->childPins.size());

    // Assume the pointer to buff moved, and re-set each child pin's buffer.
    for (size_t i = 0, j = 0; i < vectorPin->childPins.size() && j < buff.size(); i++, j+= elementSize) {
        fixPointersCb((void*)&buff[j], &vectorPin->childPins[i]);
    }

    vectorPin->RecacheInputConnections();

    if (options.onSizeChanged) {
        options.onSizeChanged(this);
    }
}