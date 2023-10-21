#include "seam/pins/iOutPinnable.h"
#include "seam/pins/pinOutput.h"

using namespace seam::pins;

PinOutput* IOutPinnable::FindPinOut(IOutPinnable* pinnable, PinId id) {
    size_t size;
    PinOutput* pins = pinnable->PinOutputs(size);
    for (size_t i = 0; i < size; i++) {
        if (pins[i].id == id) {
            return &pins[i];
        }

        // Recursively check children pins.
        PinOutput* child = FindPinOut(&pins[i], id);
        if (child != nullptr) {
            return child;
        }
    }
    return nullptr;
}