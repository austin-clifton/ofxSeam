#include "videoPlayer.h"
#include "seam/imguiUtils/properties.h"

using namespace seam::nodes;
using namespace seam::pins;

VideoPlayer::VideoPlayer() : INode("Video Player") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL | NodeFlags::UPDATES_OVER_TIME);
    gui_display_fbo = &fbo;
	windowFbos.push_back(WindowRatioFbo(&fbo, &pinOutFbo));
}

VideoPlayer::~VideoPlayer() {

}

PinInput* VideoPlayer::PinInputs(size_t& size) {
    // TODO: return your list of input pins here, if any, along with their size.
    size = pinInputs.size();
    return pinInputs.data();
}

PinOutput* VideoPlayer::PinOutputs(size_t& size) {
    // TODO: return your list of output pins here, if any, along with their size.
    size = 1;
    return &pinOutFbo;
}

void VideoPlayer::Setup(SetupParams* params) {
    LoadAndPlayVideo();
}

void VideoPlayer::Update(UpdateParams* params) {
    videoPlayer.setSpeed(playbackSpeed);
    videoPlayer.update();
}

void VideoPlayer::Draw(DrawParams* params) {
    fbo.begin();
    fbo.clearColorBuffer(ofFloatColor(0.f));

    videoPlayer.draw(0, 0, fbo.getWidth(), fbo.getHeight());

    fbo.end();
}

bool VideoPlayer::GuiDrawPropertiesList(UpdateParams* params) {
    if (props::DrawTextInput("Video Path", videoPath)) {
        LoadAndPlayVideo();
        return true;
    }
    return false;
}

void VideoPlayer::LoadAndPlayVideo() {
    if (videoPlayer.load(videoPath)) {
        videoPlayer.setVolume(0.f);
        videoPlayer.play();
    }
}

std::vector<seam::props::NodeProperty> VideoPlayer::GetProperties() {
	std::vector<props::NodeProperty> properties;
	
	properties.push_back(props::SetupStringProperty("Video Path", [this](size_t& size) {
		size = 1;
		return &videoPath;
	}, [this](std::string* newName, size_t size) {
		assert(size == 1);
		videoPath = *newName;
        LoadAndPlayVideo();
	}));

	return properties;
}