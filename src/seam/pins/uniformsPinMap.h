#pragma once

#include "seam/pins/pinInput.h"
#include "seam/include.h"
#include "ofMain.h"

namespace seam::pins {
    /// @brief Convenience container for PinInput struct which exposes shader uniforms as child Pins.
    class UniformsPinMap {
    public:
        UniformsPinMap(nodes::INode* node);

        PinInput SetupUniformsPin(const std::string_view name);

        void UpdatePins(ofShader& shader);

        void SetUniforms(ofShader& shader);

    private:
        nodes::INode* node;
        pins::PinId uniformsPinId;

        /// @brief Space for uniform pin data is allocated to this buffer.
        std::vector<char> uniformsBuffer;
    };

}