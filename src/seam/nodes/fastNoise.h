#pragma once

#include "seam/include.h"

using namespace seam::pins;

namespace {
    std::array<const char*, 6> fnlNoiseTypeNames = {
        "OpenSimplex", "OpenSimplex2s", "Cellular", "Perlin", "Value Cubic", "Value" 
    };

    std::array<const char*, 3> fnlRotationTypeNames = {
        "None", "Improve XY Planes", "Improve XZ Planes"
    };

    std::array<const char*, 6> fnlFractalTypeNames = {
        "None", "FBM", "Ridged", "Ping Pong", "Domain Warp Progressive", "Domain Warp Independent"
    };

    std::array<const char*, 4> fnlCellularDistanceTypeNames = {
        "Euclidean", "Euclidean SQ", "Manhattan", "Hybrid"
    };

    std::array<const char*, 7> fnlCellularReturnTypeNames = {
        "Cell Value", "Distance", "Distance2", "Distance2Add", "Distance2Sub", "Distance2Mul", "Distance2Div"
    };

    std::array<const char*, 3> fnlDomainWarpTypeNames = {
        "Open Simplex2", "Open Simplex2 Reduced", "Basic Grid"
    };
}

namespace seam::nodes {
    /// @brief Seam Node integration for FastNoiseLite, see:
    /// https://github.com/Auburn/FastNoiseLite/tree/master
	class FastNoise : public INode {
	public:
		FastNoise();

        void Setup(SetupParams* params) override;

		void Draw(DrawParams* params) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
        void ResizeFbo();
        bool ReloadShaders();

        /// @brief FastNoiseLite options
        struct FnlOption {
            FnlOption(const char* _optionName, const char* _uniformName, int _max, const char** _valueNames) {
                maxValue = _max;
                uniformName = _uniformName;
                optionName = _optionName;
                valueNames = _valueNames;
                meta.min = 0;
                meta.max = _max;
            }

            int currValue = 0;
            int maxValue;
            std::string uniformName;
            std::string_view optionName;
            // The length of this array is inferred by the maxValue
            const char** valueNames;
            PinIntMeta meta;
        };

        std::array<FnlOption, 6> fnlOptions = {
            FnlOption("Noise Type", "noiseType", 
                fnlNoiseTypeNames.size() - 1, fnlNoiseTypeNames.data()),
            FnlOption("Rotation Type", "rotationType", 
                fnlRotationTypeNames.size() - 1, fnlRotationTypeNames.data()),
            FnlOption("Fractal Type", "fractalType", 
                fnlFractalTypeNames.size() - 1, fnlFractalTypeNames.data()),
            FnlOption("Cellular Distance Type", "cellularDistanceType", 
                fnlCellularDistanceTypeNames.size() - 1, fnlCellularDistanceTypeNames.data()),
            FnlOption("Cellular Return Type", "cellularReturnType", 
                fnlCellularReturnTypeNames.size() - 1, fnlCellularReturnTypeNames.data()),
            FnlOption("Domain Warp Type", "domainWarpType", 
                fnlDomainWarpTypeNames.size() - 1, fnlDomainWarpTypeNames.data()),
        };

        // Global configuration
        glm::ivec2 resolution = glm::ivec2(1024, 1024);
        int vSplits = 1;
        int hSplits = 1;
        int seed = 1337;
        float frequency = 8.0f;
        /// @brief Use an internal timer so animation can be smoothly paused and unpaused.
        float internalTime = 0.f;
        float speed = 1.f;
        bool animateOverTime = true;
        bool enableDomainWarp = false;

        // Fractal type configuration
        int octaves = 3;
        float lacunarity = 2.0f;
        float gain = 0.5f;

        // The octave weighting for all non Domain Warp fractal types.
        float weightedStrength = 0.0f;

        float pingPongStrength = 2.0f;

        // Will cause artifacts if set > 1.f
        float cellularJitterMod = 1.0f;

        // "The maximum warp distance from original position when using fnlDomainWarp..."
        float domainWarpAmp = 1.f;

		ofFbo fbo;
		ofShader shader;

        // Pins are set up in the constructor!            
        std::array<PinInput, 21> pinInputs;

        PinOutput pinOutFbo = pins::SetupOutputStaticFboPin(this, &fbo, pins::PinType::FBO_RGBA, "Output");
	};
}