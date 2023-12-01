#include "range.h"

using namespace seam;
using namespace seam::nodes;

namespace {
	float MapRange(float v, float min_in, float max_in, float min_out, float max_out) {
		// clamp and map input to 0..1 range
		float nv = std::max(min(v, max_in), min_in);
		nv = (nv - min_in) / (max_in - min_in);
		// map 0..1 input range to output range
		return min_out + (max_out - min_out) * nv;
	}
}

Range::Range() : INode("Range") {

}

Range::~Range() {

}

PinInput* Range::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return &pin_inputs[0];
}

PinOutput* Range::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_value;
}

void Range::Update(UpdateParams* params) {
	value = MapRange(
		inValue,
		inRangeMin,
		inRangeMax,
		outRangeMin,
		outRangeMax
	);

	printf("range: %f\n", value);
	params->push_patterns->Push(pin_out_value, &value, 1);
}