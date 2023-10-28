#include "seam/pins/pinInput.h"
#include "seam/pins/pinConnection.h"

using namespace seam::pins;

PinInput::~PinInput() {
    // PinInputs are nice and clean up pointer refs to themselves.
    // Make sure the connection to this input is severed.
    if (connection != nullptr) {
        PinInput* pinIn = this;
        auto& conns = connection->connections;
        auto it = std::find_if(conns.begin(), conns.end(), [pinIn](const PinConnection& conn) {
            return conn.input == pinIn;
        });
        if (it != connection->connections.end()) {
            connection->connections.erase(it);
        }
        connection = nullptr;
    }
}