
#include "seam/nodes/random.h"

using namespace seam;
using namespace seam::nodes;

Random::Random() : INode("Random") {
    CalculateOutput();
}

PinInput* Random::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

PinOutput* Random::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutValue;
}

void Random::Update(UpdateParams* params) {
	params->push_patterns->Push(pinOutValue, &value, 1);
}

std::vector<props::NodeProperty> Random::GetProperties() {
    std::vector<props::NodeProperty> props;

	props.push_back(props::SetupUintProperty("Num Coords", 
		[this](size_t& size) {
			size = 1;
			return &numCoords;
		}, [this](uint32_t* newNumCoords, size_t size) {
			assert(size == 1);
			numCoords = std::min(*newNumCoords, 4U);
			pinOutValue.SetNumCoords(numCoords);
            CalculateOutput();
		})
	);

	return props;
}

void Random::CalculateOutput() {
    for (uint32_t i = 0; i < numCoords; i++) {
        float r = (float)(rand() / (float)(RAND_MAX));
        value[i] = std::min(range[0], range[1]) + r * std::abs(range[1] - range[0]);
    }
}