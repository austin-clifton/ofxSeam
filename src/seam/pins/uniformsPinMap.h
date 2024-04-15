#pragma once

#include <unordered_set>

#include "seam/pins/pinInput.h"
#include "seam/include.h"
#include "ofMain.h"

namespace seam::pins {
    /// @brief Convenience container for PinInput struct which exposes shader uniforms as child Pins.
    class UniformsPinMap {
    public:
        UniformsPinMap(nodes::INode* node, ofShader* shader);

        PinInput SetupUniformsPin(const std::string_view name);

        /// @brief Should be called after uniforms change (when loading / reloading the shader)
        /// @param blacklist Uniform names in the blacklist won't be exposed as pins
        void UpdatePins(const std::unordered_set<std::string>& blacklist = {});

        /// @brief Should be called whenever pins change and uniform values should be updated.
        void SetUniforms();

    private:
        nodes::INode* node;
        ofShader* shader;
        std::string uniformsPinName;

        /// @brief Space for uniform pin data is allocated to this buffer.
        std::vector<char> uniformsBuffer;
    };

}