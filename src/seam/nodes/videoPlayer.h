#pragma once

#include "seam/include.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
    /// @brief Loads and plays video files into an FBO.
    /// TODO pass around the video texture instead of rendering to an FBO
	class VideoPlayer : public INode {
	public:
		VideoPlayer();
		~VideoPlayer();
        
		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		void Setup(SetupParams* params) override;

        void Update(UpdateParams* params) override;

		void Draw(DrawParams* params) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

		std::vector<props::NodeProperty> GetProperties() override;

	private:
        void LoadAndPlayVideo();

        std::string videoPath;
        ofVideoPlayer videoPlayer;
		ofFbo fbo;

        float playbackSpeed = 1.0f;

        std::array<PinInput, 1> pinInputs = {
			pins::SetupInputPin(PinType::FLOAT, this, &playbackSpeed, 1, "Playback Speed"),
        };

        PinOutput pinOutFbo = pins::SetupOutputStaticFboPin(
            this, &fbo, pins::PinType::FBO_RGBA16F, "Video Feed"
        );
	};
}
