#include "seam/nodes/nodeTemplate.h"

using namespace seam::nodes;
using namespace seam::pins;

// TODO: Replace the name "Node Template" with your custom Node's user-facing name.
NodeTemplate::NodeTemplate() : INode("Node Template") {
    // TODOs:
    // - Configure node flags
    // - Configure gui display FBO, required if this node is visual
    // - Configure optional window size synced FBOs
    // - Don't do any heavy setup configuration here; do that in Setup() instead.

    /* TODO: Uncomment if this node has a visual output, otherwise delete.
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

    // TODO: Visual nodes MUST provide a gui display FBO (usually an output FBO).
    // Configure the gui_display_fbo to be your output FBO (whatever it's named) here.
    gui_display_fbo = &fbo;
    
    */

    // TODO: If any FBOs should sync to the window's size, add them here.
	// windowFbos.push_back(WindowRatioFbo(&fbo));
}

NodeTemplate::~NodeTemplate() {

}

PinInput* NodeTemplate::PinInputs(size_t& size) {
    // TODO: return your list of input pins here, if any, along with their size.
    size = 0;
    return nullptr;
}

PinOutput* NodeTemplate::PinOutputs(size_t& size) {
    // TODO: return your list of output pins here, if any, along with their size.
    size = 0;
    return nullptr;
}

void NodeTemplate::Setup(SetupParams* params) {
    // TODOs:
    // - Allocate GPU resources and handle any other heavy setup operations.
    // - Setup() is called right after instantiation. Serialized Pin values will NOT be set by this time!
}

// void NodeTemplate::Update(UpdateParams* params) { }

// void NodeTemplate::Draw(DrawParams* params) { }

// bool NodeTemplate::GuiDrawPropertiesList(UpdateParams* params) { }