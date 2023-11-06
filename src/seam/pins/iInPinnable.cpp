#include "seam/pins/iInPinnable.h"
#include "seam/pins/pinInput.h"
#include "seam/pins/pinConnection.h"

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

void IInPinnable::RecacheInputConnections() {
	size_t size;
	PinInput* pins = PinInputs(size);
	RecacheInputConnections(pins, size);
}

void IInPinnable::RecacheInputConnections(PinInput* pins, size_t size) {
    for (size_t i = 0; i < size; i++) {
		// If the Input pin is connected, re-cache the PinOutput's pointer to the PinInput.
		if (pins[i].connection != nullptr) {
			// Find the input pin in the Output pin's connections
			std::vector<pins::PinConnection>& connections = pins[i].connection->connections;
			for (size_t j = 0; j < connections.size(); j++) {
				if (connections[j].inputPinId == pins[i].id) {
					connections[j].input = &pins[i];
					break;
				}
			}
		}

		// Recurse for child hierarchies
		size_t childrenSize;
		PinInput* children = pins[i].PinInputs(childrenSize);
		if (children != nullptr) {
			RecacheInputConnections(children, childrenSize);
		}
	}
}
