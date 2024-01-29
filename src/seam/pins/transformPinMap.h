
#pragma once

#include "seam/pins/pinInput.h"
#include "ofMain.h"

namespace seam::pins {
    /// @brief Maps an ofNode transform to a struct pin with position, rotation, and scale.
    class TransformPinMap {
    public:
        PinInput Setup(nodes::INode* node, ofNode* transform, const std::string_view name = "Transform");

    private:
        ofNode* transform = nullptr;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale = glm::vec3(1.f);
    };

}