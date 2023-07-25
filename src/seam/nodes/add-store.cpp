#include "add-store.h"

using namespace seam;
using namespace seam::nodes;

AddStore::AddStore() : INode("Add Store") {

}

AddStore::~AddStore() {

}

IPinInput** AddStore::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return &pin_inputs[0];
}

PinOutput* AddStore::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_value;
}

void AddStore::Update(UpdateParams* params) {
	value += pin_incrementer[0];
	// printf("addstore: %f\n", value);
	params->push_patterns->Push(pin_out_value, &value, 1);
}