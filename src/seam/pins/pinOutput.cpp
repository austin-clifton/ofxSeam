#include "seam/pins/pinOutput.h"
#include "seam/pins/pinConnection.h"
#include "seam/include.h"

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

void PinOutput::SetType(PinType t) {
    if (type == t) {
        return;
    }

    type = t;
    for (auto& c : connections) {
        c.RecacheConverts();
    }
}

void PinOutput::Reconnect(seam::pins::PushPatterns* pushPatterns) {
    if (!onConnected) {
        return;
    }

    PinConnectedArgs args;
    args.pinOut = this;
    args.pushPatterns = pushPatterns;
    for (auto conn : connections) {
        args.pinIn = conn.pinIn;
        OnConnected(args);
    }
}