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
		params->push_patterns->Push(PinOutSelectionF, &currentStateF, 1);
		params->push_patterns->PushFlow(pinOutChangedEvent);
		currentStateDuration = 0.f;
	}
}

bool Markov::GuiDrawPropertiesList() {
	bool nodesCountChanged = ImGui::DragInt("States Count", &nodesCount, 1.f, 2);
	nodesCount = max(2, nodesCount);
	if (nodesCountChanged) {
		Reconfigure();
	}

	ImGui::DragInt("Selected Node", &gui.selectedNode, 1.f, 0, nodesCount - 1);
	gui.selectedNode = max(0, min(nodesCount - 1, gui.selectedNode));

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

std::vector<NodeProperty> Markov::GetProperties() {
	std::vector<NodeProperty> properties;
	properties.push_back(NodeProperty("States Count", NodePropertyType::PROP_INT, &nodesCount, 1));

	for (size_t i = 0; i < markovNodes.size(); i++) {
		auto& node = markovNodes[i];
		std::string nodeName = "Node " + std::to_string(i) + " ";
		properties.push_back(NodeProperty(nodeName + "Duration", NodePropertyType::PROP_FLOAT, &node.duration, 1));
		properties.push_back(NodeProperty(nodeName + "Weights", NodePropertyType::PROP_FLOAT, node.transitionWeights.data(), node.transitionWeights.size()));
	}

	return properties;
}

void Markov::Reconfigure() {
	markovNodes.resize(nodesCount);
	for (auto& node : markovNodes) {
		node.transitionWeights.resize(nodesCount, 0.f);
	}
}