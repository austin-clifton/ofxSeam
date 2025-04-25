#include "seam/pins/transformPinMap.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

PinInput TransformPinMap::Setup(nodes::INode* node, ofNode* _transform, const std::string_view name) {
    // Should only be called once per TransformPin
    assert(transform == nullptr);
    transform = _transform;

    PinInput pin = SetupInputPin(PinType::Struct, node, nullptr, 0, name);
    std::vector<PinInput> children;

    children.push_back(SetupInputPin(PinType::Float, node, &position, 1, "Position", 
        PinInOptions::WithCoords(3, [this]() { transform->setPosition(position); })));
    children.push_back(SetupInputPin(PinType::Float, node, &rotation, 1, "Rotation", 
        PinInOptions::WithCoords(3, [this]() { transform->setOrientation(rotation); })));
    children.push_back(SetupInputPin(PinType::Float, node, &scale, 1, "Scale", 
        PinInOptions::WithCoords(3, [this]() { transform->setScale(scale); })));
        
    pin.SetChildren(std::move(children));
    return pin;
}