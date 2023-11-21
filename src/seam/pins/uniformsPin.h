#pragma once

#include "pinInput.h"
#include "ofMain.h"

namespace seam::pins {
    /// @brief Convenience container for PinInput struct which exposes shader uniforms as child Pins.
    class UniformsPin {
    public:
        PinInput SetupUniformsPin(nodes::INode* node, const std::string_view name);

        void UpdateUniforms(PinInput* uniformsPin, ofShader& shader);

        void SetShaderUniforms(PinInput* uniformsPin, ofShader& shader);

    private:
        /// @brief Space for uniform pin data is allocated to this buffer.
        std::vector<char> uniformsBuffer;
    };

}