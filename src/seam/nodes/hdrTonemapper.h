#pragma once

#include "seam/nodes/iNode.h"

using namespace seam::pins;

namespace {
	const uint16_t bloomDownScales = 5;
}

namespace seam::nodes { 
    /// @brief Maps a 16-bit RGBA frame buffer to an 8-bit RGBA frame buffer,
	/// with tone-mapping and bloom.
	class HdrTonemapper : public INode {
	public:
		HdrTonemapper();
		virtual ~HdrTonemapper() override;

        void Setup(SetupParams* params) override;

		void Update(UpdateParams* params) override;

		void Draw(DrawParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

        void OnPinConnected(PinConnectedArgs args) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		void OnWindowResized(glm::uvec2 resolution) override;

	private:
        bool ReloadShaders();
		void RebindTexture();
		void OnResolutionChanged();
		void SetupBloomTextures();
		void RebindBloomTextures();
		glm::ivec2 DownscaledRes();

		glm::ivec2 resolution = glm::vec2(1920, 1080);
		float gamma = 2.2f;

		/// @brief Input FBO
		ofFbo* hdrFbo;

		/// @brief Post-processed input FBOs with only bright areas.
		/// These are downscaled and then ping-pong'd for bloom.
		std::array<ofFbo, bloomDownScales> bloomFbos;
		std::array<ofFbo, bloomDownScales> bloomFbosBack;

        /// @brief Output FBO
		ofFbo tonemappedFbo;

		ofShader brightPassShader;
		ofShader blurShader;
        ofShader tonemapShader;

		std::array<PinInput, 3> pinInputs = {
			pins::SetupInputPin(PinType::FBO_RGBA16F, this, &hdrFbo, 1, "HDR FBO",
				PinInOptions(std::bind(&HdrTonemapper::RebindTexture, this))),
			pins::SetupInputPin(PinType::INT, this, &resolution, 2, "Resolution", 
				PinInOptions(std::bind(&HdrTonemapper::OnResolutionChanged, this))),
			pins::SetupInputPin(PinType::FLOAT, this, &gamma, 1, "Gamma"),
		};
        
        PinOutput pinOutFbo = pins::SetupOutputPin(this, pins::PinType::FBO_RGBA, "Tonemapped FBO");
	};
}