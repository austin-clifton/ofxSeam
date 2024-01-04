#pragma once

#include "seam/nodes/iNode.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
    /// @brief Template for writing your own Nodes. Includes some handy TODOs so you know what to do.
    /// TODO: Inherit IDynamicPinsNode instead of INode if the Node has dynamic pins which need to serialize/deserialize.
	class NodeTemplate : public INode {
	public:
		NodeTemplate();
		~NodeTemplate();
        
		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		void Setup(SetupParams* params) override;

        // TODO uncomment if you want to run an update loop every time one of this pin's inputs changes.
        // void Update(UpdateParams* params) override;

        // TODO uncomment if your node is visual.
		// void Draw(DrawParams* params) override;

        // TOD uncomment if you want to draw non-pin properties in the GUI, to be set by users.
		// bool GuiDrawPropertiesList(UpdateParams* params) override;

	private:
        // TODO: declare input / output pins and data here.
	};
}
