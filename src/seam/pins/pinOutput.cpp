#include "seam/pins/pinOutput.h"
#include "seam/pins/pinConnection.h"
#include "seam/nodes/iNode.h"

using namespace seam::pins;

void PinOutput::DirtyConnections() {
    for (PinConnection& conn : connections) {
        conn.input->node->SetDirty();
    }
}