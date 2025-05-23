#include "seam/factory.h"
#include "seam/hash.h"

#include "seam/nodes/addStore.h"
#include "seam/nodes/audioAnalyzer.h"
#include "seam/nodes/channelMap.h"
#include "seam/nodes/computeParticles.h"
#include "seam/nodes/cos.h"
#include "seam/nodes/fastNoise.h"
#include "seam/nodes/feedback.h"
#include "seam/nodes/gate.h"
#include "seam/nodes/hdrTonemapper.h"
#include "seam/nodes/markov.h"
#include "seam/nodes/midiIn.h"
#include "seam/nodes/multiTrigger.h"
#include "seam/nodes/noise.h"
#include "seam/nodes/notesPrinter.h"
#include "seam/nodes/percussiveTrigger.h"
#include "seam/nodes/range.h"
#include "seam/nodes/saw.h"
#include "seam/nodes/shader.h"
#include "seam/nodes/step.h"
#include "seam/nodes/threshold.h"
#include "seam/nodes/timer.h"
#include "seam/nodes/toggle.h"
#include "seam/nodes/valueNoise.h"
#include "seam/nodes/videoPlayer.h"

using namespace seam;

EventNodeFactory::EventNodeFactory() {
	// register seam-internal nodes here
	Register(MakeCreate<nodes::AddStore>());
	#if BUILD_AUDIO_ANALYSIS
	Register(MakeCreate<nodes::AudioAnalyzer>());
	#endif
	// Register(MakeCreate<nodes::ComputeParticles>());
	Register(MakeCreate<nodes::Cos>());
	Register(MakeCreate<nodes::FastNoise>());
	Register(MakeCreate<nodes::Feedback>());
	Register(MakeCreate<nodes::Gate>());
	Register(MakeCreate<nodes::HdrTonemapper>());
	Register(MakeCreate<nodes::Markov>());
	// Register(MakeCreate<nodes::MidiIn>());
	Register(MakeCreate<nodes::MultiTrigger>());
	Register(MakeCreate<nodes::Noise>());
	Register(MakeCreate<nodes::NotesPrinter>());
	Register(MakeCreate<nodes::PercussiveTrigger>());
	Register(MakeCreate<nodes::Range>());
	Register(MakeCreate<nodes::Saw>());
	Register(MakeCreate<nodes::Shader>());
	Register(MakeCreate<nodes::Step>());
	Register(MakeCreate<nodes::Threshold>());
	Register(MakeCreate<nodes::Timer>());
	Register(MakeCreate<nodes::Toggle>());
	Register(MakeCreate<nodes::ValueNoise>());
	Register(MakeCreate<nodes::VideoPlayer>());

	// TODO register more seam internal generators here


	IdsDistributor::GetInstance().ResetIds();
}

EventNodeFactory::~EventNodeFactory() {
	// nothing to do?
}

nodes::INode* EventNodeFactory::Create(seam::nodes::NodeId node_id) {
	if (!generators_sorted) {
		std::sort(generators.begin(), generators.end());
		generators_sorted = true;
	}
	// find the generator
	auto it = std::lower_bound(generators.begin(), generators.end(), node_id);
	if (it != generators.end() && it->node_id == node_id) {
		nodes::INode* node = it->Create();
		assert(node);

		// make sure each input pin has this node set as its parent
		size_t size;
		PinInput* pin_inputs = node->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			pin_inputs[i].node = node;
		}

		return node;
	} else {
		printf("node with node id %llu not found, did you register it?", node_id);
		return nullptr;
	}
}

bool EventNodeFactory::Register(EventNodeFactory::CreateFunc&& Create) {
	Generator gen;
	// TODO it'd be nice to stack alloc dummy nodes instead but probably doesn't matter that much;
	// this should only run once per node type
	std::unique_ptr<nodes::INode> n(Create());
	gen.node_name = n->NodeName();
	gen.node_id = SCHash(gen.node_name.data(), gen.node_name.length());

	// make sure it's not already registered before continuing
	if (std::find(generators.begin(), generators.end(), gen.node_id) != generators.end()) {
		// node id already registered, probably a bug
		printf("node with name %s and id %llu already registered\n", 
			gen.node_name.data(), gen.node_id);
		return false;
	}
	
	size_t size;

	// read inputs
	{
		PinInput* inputs = n->PinInputs(size);
		for (size_t i = 0; i < size; i++) {
			PinType type = inputs[i].type;
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
			PinType type = outputs[i].type;
			if (std::find(gen.pin_outputs.begin(), gen.pin_outputs.end(), type) == gen.pin_outputs.end()) {
				gen.pin_outputs.push_back(type);
			}
		}
	}

	gen.Create = std::move(Create);
	generators.push_back(std::move(gen));
	generators_sorted = false;
	return true;
}

seam::nodes::NodeId EventNodeFactory::DrawCreatePopup(PinType input_type, PinType output_type) {
	if (!generators_sorted || guiGenerators.size() != generators.size()) {
		std::sort(generators.begin(), generators.end());

		guiGenerators.clear();
		for (size_t i = 0; i < generators.size(); i++) {
			guiGenerators.push_back(&generators[i]);
		}

		std::sort(guiGenerators.begin(), guiGenerators.end(), [](const Generator* a, const Generator* b) -> bool {
			return a->node_name < b->node_name;
		});

		generators_sorted = true;
	}

	// loop through each Generator that's been registered with the EventNodeFactory,
	// and make a selectable menu item for it
	nodes::NodeId new_node_id = 0;
	for (auto gen : guiGenerators) {
		// TODO filter input / output types, only enable items which match the filter

		// MenuItem() will return true if this menu item was selected
		if (ImGui::MenuItem(gen->node_name.data())) {
			new_node_id = gen->node_id;
		}
	}

	return new_node_id;
}
