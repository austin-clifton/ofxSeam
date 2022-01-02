#include "factory.h"
#include "hash.h"

#include "event-nodes/texgen-perlin.h"
#include "event-nodes/cos.h"
#include "event-nodes/node-shader.h"

using namespace seam;

EventNodeFactory::EventNodeFactory() {
	// register seam-internal nodes here
	Register([] {
		return new seam::TexgenPerlin();
	});

	Register([] {
		return new seam::Cos();
	});

	Register([] {
		return new seam::ShaderNode();
	});

	// TODO register more seam internal generators here
}

EventNodeFactory::~EventNodeFactory() {
	// nothing to do?
}

IEventNode* EventNodeFactory::Create(NodeId node_id) {
	if (!generators_sorted) {
		std::sort(generators.begin(), generators.end());
		generators_sorted = true;
	}
	// find the generator
	auto it = std::lower_bound(generators.begin(), generators.end(), node_id);
	if (it != generators.end() && it->node_id == node_id) {
		IEventNode* node = it->Create();
		assert(node);

		// make sure each input pin has this node set as its parent
		size_t size;
		IPinInput** pin_inputs = node->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			pin_inputs[i]->node = node;
		}

		return node;
	} else {
		printf("node with node id %u not found, did you register it?", node_id);
		return nullptr;
	}
}

bool EventNodeFactory::Register(EventNodeFactory::CreateFunc&& Create) {
	Generator gen;
	// TODO it'd be nice to stack alloc dummy nodes instead but probably doesn't matter that much;
	// this should only run once per node type
	std::unique_ptr<IEventNode> n(Create());
	gen.node_name = n->NodeName();
	gen.node_id = SCHash(gen.node_name.data(), gen.node_name.length());

	// make sure it's not already registered before continuing
	if (std::find(generators.begin(), generators.end(), gen.node_id) != generators.end()) {
		// node id already registered, probably a bug
		printf("node with name %s and id %u already registered\n", 
			gen.node_name.data(), gen.node_id);
		return false;
	}
	
	size_t size;

	// read inputs
	{
		IPinInput** inputs = n->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			PinType type = inputs[i]->type;
			// filter only for types that aren't already in the list
			if (std::find(gen.pin_inputs.begin(), gen.pin_inputs.end(), type) == gen.pin_inputs.end()) {
				gen.pin_inputs.push_back(type);
			}
		}
	}

	// read outputs
	{
		PinOutput* outputs = n->PinOutputs(size);
		for (size_t i = 0; i < size; i++) {
			PinType type = outputs[i].pin.type;
			if (std::find(gen.pin_outputs.begin(), gen.pin_outputs.end(), type) == gen.pin_outputs.end()) {
				gen.pin_outputs.push_back(type);
			}
		}
	}

	gen.Create = std::move(Create);
	generators.push_back(std::move(gen));
	return true;
}

NodeId EventNodeFactory::DrawCreatePopup(PinType input_type, PinType output_type) {
	if (!generators_sorted) {
		std::sort(generators.begin(), generators.end());
	}

	// loop through each Generator that's been registered with the EventNodeFactory,
	// and make a selectable menu item for it
	NodeId new_node_id = 0;
	for (auto gen : generators) {
		// TODO filter input / output types, only enable items which match the filter

		// MenuItem() will return true if this menu item was selected
		if (ImGui::MenuItem(gen.node_name.data())) {
			new_node_id = gen.node_id;
		}
	}

	return new_node_id;
}
