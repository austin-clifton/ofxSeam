#include "noise.h"

using namespace seam;
using namespace seam::nodes;

Noise::Noise() : INode("Noise") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);
}

Noise::~Noise() {

}

pins::PinInput* Noise::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

pins::PinOutput* Noise::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutNoiseVal;
}

void Noise::Update(UpdateParams* params) {
	glm::vec4 noises;
	for (uint32_t i = 0; i < numCoords; i++) {
		samples[i] = samples[i] + params->delta_time * speedMultiplier;
		noises[i] = minN + ofNoise(samples[i]) * (maxN - minN);
	}

	params->push_patterns->Push(pinOutNoiseVal, &noises, 1);
}

std::vector<props::NodeProperty> Noise::GetProperties() {
	std::vector<props::NodeProperty> props;

	props.push_back(props::SetupUintProperty("Num Coords", 
		[this](size_t& size) {
			size = 1;
			return &numCoords;
		}, [this](uint32_t* newNumCoords, size_t size) {
			assert(size == 1);
			numCoords = std::min(*newNumCoords, 4U);
			pinOutNoiseVal.SetNumCoords(numCoords);
		})
	);

	return props;
}