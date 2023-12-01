#include "markov.h"

using namespace seam;
using namespace seam::nodes;

Markov::Markov() : INode("Markov") {
	flags = (NodeFlags)(NodeFlags::UPDATES_OVER_TIME | flags);
	Reconfigure();
}

Markov::~Markov() {

}

void Markov::Update(UpdateParams* params) {
	currentStateDuration += params->delta_time;

	auto& selectedNode = markovNodes[currentState];
	if (currentStateDuration > selectedNode.duration) {
		// Time for a state transition!

		// First add up all the weights for this Node.
		float weightsTotal = std::accumulate(selectedNode.transitionWeights.begin(), selectedNode.transitionWeights.end(), 0.f);

		// Now produce a random number with a maximum value of the weightsTotal.
		float f = ofRandom(weightsTotal);

		// Now figure out which choice "bucket" f falls into.
		int newState = 0;
		for (; newState < nodesCount - 1; newState++) {
			f = f - selectedNode.transitionWeights[newState];
			// If we cross from positive to negative, f falls into the weighted bucket for this selection.
			if (f < 0.f) {
				break;
			}
		}

		printf("Markov transitioning from state %d to state %d\n", currentState, newState);
		currentState = newState;

		float currentStateF = currentState;

		params->push_patterns->Push(pinOutSelection, &currentState, 1);
		params->push_patterns->PushFlow(pinOutChangedEvent);
		currentStateDuration = 0.f;
	}
}

bool Markov::GuiDrawPropertiesList(UpdateParams* params) {
	bool nodesCountChanged = ImGui::DragInt("States Count", &nodesCount, 1.f, 2);
	nodesCount = std::max(2, nodesCount);
	if (nodesCountChanged) {
		Reconfigure();
	}

	ImGui::DragInt("Selected Node", &gui.selectedNode, 1.f, 0, nodesCount - 1);
	gui.selectedNode = std::max(0, std::min(nodesCount - 1, gui.selectedNode));

	auto& selectedNode = markovNodes[gui.selectedNode];

	ImGui::DragFloat("Duration", &selectedNode.duration, 1.f, 0.01f);

	for (size_t i = 0; i < selectedNode.transitionWeights.size(); i++) {
		std::stringstream label;
		label << "Transition " << i << " Weight";
		ImGui::DragFloat(label.str().c_str(), &selectedNode.transitionWeights[i], 1.f, 0.f);
	}

	return false;
}

pins::PinInput* Markov::PinInputs(size_t& size) {
	size = 0;
	return nullptr;
}

pins::PinOutput* Markov::PinOutputs(size_t& size) {
	size = 3;
	return &pinOutSelection;
}

std::vector<props::NodeProperty> Markov::GetProperties() {
	using namespace seam::props;

	std::vector<NodeProperty> properties;
	properties.push_back(
		SetupIntProperty("States Count", [this](size_t& size) {
			size = 1;
			return &nodesCount;
		}, [this](int32_t* newCount, size_t size) {
			assert(size == 1);
			nodesCount = std::max(2, *newCount);
			Reconfigure();
		})
	);

	for (size_t i = 0; i < markovNodes.size(); i++) {
		auto& node = markovNodes[i];
		std::string nodeName = "Node " + std::to_string(i) + " ";

		properties.push_back(
			SetupFloatProperty(nodeName + "Duration", [this, i](size_t& size) {
				size = 1;
				return &markovNodes[i].duration;
			}, [this, i](float* newDuration, size_t size) {
				assert(size == 1);
				markovNodes[i].duration = *newDuration;
			}) 
		);

		properties.push_back(
			SetupFloatProperty(nodeName + "Weights", [this, i](size_t& size) {
				size = markovNodes[i].transitionWeights.size();
				return markovNodes[i].transitionWeights.data();
			}, [this, i](float* newWeights, size_t size) {
				markovNodes[i].transitionWeights.resize(size);
				std::copy(newWeights, newWeights + size, markovNodes[i].transitionWeights.data());
			}) 
		);
	}

	return properties;
}

void Markov::Reconfigure() {
	markovNodes.resize(nodesCount);
	for (auto& node : markovNodes) {
		node.transitionWeights.resize(nodesCount, 0.f);
	}
}