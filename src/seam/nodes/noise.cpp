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
	sample = sample + params->delta_time * speedMultiplier;
	float n = minN + ofNoise(sample) * (maxN - minN);
	params->push_patterns->Push(pinOutNoiseVal, &n, 1);
}
