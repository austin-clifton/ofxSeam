#include "seam/nodes/iNode.h"
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

void PinInput::SetEnabled(bool enabled) {
    if (IsEnabled() == enabled) {
        return;
    } else {
        if (enabled) {
            flags = (flags & ~PinFlags::Disabled);
        } else {
            flags = (flags | PinFlags::Disabled);
        }

        if (connection != nullptr) {
            auto parentConn = std::find(node->parents.begin(), node->parents.end(), connection->node);
            assert(parentConn != node->parents.end());
            parentConn->activeConnections += enabled * 1 + !enabled * -1;
        }
    }
}

void PinInput::SetType(PinType t) {
    if (type == t) {
        return;
    }
    
    type = t;
    if (connection != nullptr) {
        // Find this connection in the connections list and recache its convert functions.
        auto it = std::find_if(
            connection->connections.begin(), 
            connection->connections.end(), 
            [this](PinConnection conn) {
                return conn.pinIn == this;
            }
        );

        assert(it != connection->connections.end());
        it->RecacheConverts();
    }
}