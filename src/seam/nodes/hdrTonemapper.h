#pragma once

#include "seam/include.h"

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

		bool GuiDrawPropertiesList(UpdateParams* params) override;

	private:
        bool ReloadShaders();
		void RebindTexture();
		void SetupBloomTextures();
		void RebindBloomTextures();
		glm::ivec2 DownscaledRes();

		glm::ivec2 resolution = glm::vec2(1920, 1080);
		float gamma = 2.2f;

		/// @brief Input FBO
		ofFbo* hdrFbo = nullptr;

		/// @brief Post-processed input FBOs with only bright areas.
		/// These are downscaled and then ping-pong'd for bloom.
		std::array<ofFbo, bloomDownScales> bloomFbos;
		std::array<ofFbo, bloomDownScales> bloomFbosBack;

        /// @brief Output FBO
		ofFbo tonemappedFbo;

		ofShader brightPassShader;
		ofShader blurShader;
        ofShader tonemapShader;

		std::array<PinInput, 2> pinInputs = {
			pins::SetupInputPin(PinType::FBO_RGBA16F, this, &hdrFbo, 1, "HDR FBO",
				PinInOptions::WithChangedCallbacks(
					std::bind(&HdrTonemapper::RebindTexture, this),
					[this]() {
						if (hdrFbo != nullptr) {
							Seam().texLocResolver->Release(&hdrFbo->getTexture()); 
						}
					}
				)
			),
			pins::SetupInputPin(PinType::FLOAT, this, &gamma, 1, "Gamma"),
		};
        
        PinOutput pinOutFbo = pins::SetupOutputStaticFboPin(
			this, &tonemappedFbo, pins::PinType::FBO_RGBA, "Tonemapped FBO");
	};
}