#include "range.h"

using namespace seam;
using namespace seam::nodes;

const char* Range::inputValuePinName = "Input Value";

namespace {
	glm::vec4 MapRange(glm::vec4 v, glm::vec4 min_in, glm::vec4 max_in, glm::vec4 min_out, glm::vec4 max_out) {
		// clamp and map input to 0..1 range
		glm::vec4 nv = glm::max(glm::min(v, max_in), min_in);
		nv = (nv - min_in) / (max_in - min_in);
		// map 0..1 input range to output range
		return min_out + (max_out - min_out) * nv;
	}
}

Range::Range() : INode("Range") {

}

PinInput* Range::PinInputs(size_t& size) {
	size = pinInputs.size();
	return &pinInputs[0];
}

PinOutput* Range::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_value;
}

void Range::Update(UpdateParams* params) {
	glm::vec4 value = MapRange(
		inValue,
		inRangeMin,
		inRangeMax,
		outRangeMin,
		outRangeMax
	);

	params->push_patterns->Push(pin_out_value, &value, 1);
}

void Range::OnPinConnected(PinConnectedArgs args) {
	// If the value pin was connected, use the output's number of coordinates to determine
	// how many coords Range should have.
	// TODO... This isn't the right place to do this; what if the parent pin updates its numCoords after connecting?
	// There needs to be some other way for coordinate-dependent nodes/pins to handle num coords changes.

	// Can't accomodate more than 4 coords... for now?
	uint16_t numCoords = std::min(args.pinOut->NumCoords(), (uint16_t)4);

	if (args.pinIn->name == inputValuePinName) {
		for (size_t i = 0; i < pinInputs.size(); i++) {
			pinInputs[i].SetNumCoords(numCoords);
		}
	}

	pin_out_value.SetNumCoords(numCoords);
}