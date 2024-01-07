#include "seam/pins/pinOutput.h"
#include "seam/pins/pinConnection.h"
#include "seam/nodes/iNode.h"

using namespace seam::pins;

PinOutput::~PinOutput() {
    // Clean up any existing pin input references to this output pin.
    for (auto& conn : connections) {
        conn.pinIn->connection = nullptr;
    }
}

void PinOutput::DirtyConnections() {
    for (PinConnection& conn : connections) {
        conn.pinIn->node->SetDirty();
    }
}

void PinOutput::SetNumCoords(uint16_t newNumCoords) {
    numCoords = newNumCoords;
    for (auto& conn : connections) {
        conn.RecacheConverts();
    }
}