#include "seam/pins/pinInput.h"
#include "seam/pins/pinConnection.h"

using namespace seam::pins;

namespace {
    std::vector<PinConnection>::iterator FindConnection(
        std::vector<PinConnection>& conns, PinInput* pinIn
    ) {
        return std::find_if(conns.begin(), conns.end(), [pinIn](const PinConnection& conn) {
            return pinIn == conn.pinIn;
        });
    }
}

PinInput::~PinInput() {
    // PinInputs are nice and clean up pointer refs to themselves.
    // Make sure any existing connection to this input is severed.
    if (connection != nullptr) {
        auto& outputConns = connection->connections;
        auto it = FindConnection(outputConns, this);
        if (it != outputConns.end()) {
            outputConns.erase(it);
        }
    }
}

void PinInput::SetNumCoords(uint16_t _numCoords) {
    numCoords = _numCoords;
    if (connection != nullptr) {
        auto it = FindConnection(connection->connections, this);
        assert(it != connection->connections.end());
        it->RecacheConverts();
    }
}