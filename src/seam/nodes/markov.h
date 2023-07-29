#pragma once

#include "i-node.h"

using namespace seam::pins;

namespace seam::nodes {
	class Markov : public INode {
	public:
		Markov();
		~Markov();

		void Update(UpdateParams* params) override;

		bool GuiDrawPropertiesList() override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		std::vector<NodeProperty> GetProperties() override;

	private:
		void Reconfigure();

		struct MarkovNode {
			std::vector<float> transitionWeights;
			float duration = 1.f;
		};

		struct GuiState {
			int selectedNode = 0;
		};
		
		int nodesCount = 2;
		int currentState = 0;
		std::vector<MarkovNode> markovNodes;
		GuiState gui;
		
		float currentStateDuration = 0.f;

		PinOutput pinOutSelection = pins::SetupOutputPin(this, pins::PinType::INT, "Selection");
	};
}