#include "seam/pins/iInPinnable.h"
#include "seam/pins/pinInput.h"

using namespace seam::pins;

PinInput* IInPinnable::FindPinIn(IInPinnable* pinnable, PinId id) {
    size_t size;
    PinInput* pins = pinnable->PinInputs(size);
    for (size_t i = 0; i < size; i++) {
        if (pins[i].id == id) {
            return &pins[i];
        }

        // Recursively check children pins.
        PinInput* child = FindPinIn(&pins[i], id);
        if (child != nullptr) {
            return child;
        }
    }
    return nullptr;
}