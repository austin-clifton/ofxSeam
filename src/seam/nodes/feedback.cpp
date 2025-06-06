#include "seam/nodes/feedback.h"
#include "seam/shaderUtils.h"

using namespace seam;
using namespace seam::nodes;

const std::string Feedback::fboInputName = "Input FBO";

Feedback::Feedback() : INode("Feedback") {
	flags = (NodeFlags)(flags | NodeFlags::IsVisual);
	gui_display_fbo = &fbo1;
}

void Feedback::Setup(SetupParams* params) {
	ReloadShader();
}

void Feedback::Update(UpdateParams* params) {
	ofFbo* fbos = { gui_display_fbo };
	params->push_patterns->Push(pinOutFbo, &fbos, 1);
}

void Feedback::Draw(DrawParams* params) {
	ofFbo* fbo_read, *fbo_write;
	if (write_to_fbo1) {
		fbo_write = &fbo1;
		fbo_read = &fbo2;
		gui_display_fbo = &fbo1;
	} else {
		fbo_write = &fbo2;
		fbo_read = &fbo1;
		gui_display_fbo = &fbo2;
	}
	fbo_write->begin();
	fbo_write->clearColorBuffer(ofFloatColor(0.f, 1.f));

	feedback_shader.begin();

	feedback_shader.setUniform1f("decay", decay);
	feedback_shader.setUniform4f("filterColor", filterColor);
	feedback_shader.setUniform2f("feedbackOffset", feedbackOffset);
	
	// TODO if/else for radial vs non-radial modes
	feedback_shader.setUniform2f("radialFeedbackCenter", radialCenter);
	feedback_shader.setUniform1f("radialOffset", radialOffset);

	fbo_read->draw(0, 0);

	feedback_shader.end();
	fbo_write->end();

	write_to_fbo1 = !write_to_fbo1;
}

PinInput* Feedback::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* Feedback::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutFbo;
}

bool Feedback::GuiDrawPropertiesList(UpdateParams* params) {
	if (ImGui::Button("Reload Shaders")) {
		return ReloadShader();
	}

	return false;
}

bool Feedback::ReloadShader() {
	return ShaderUtils::LoadShader(feedback_shader, "screen-rect.vert", "feedback.frag");
}

void Feedback::ResizeFrameBuffers() {
	if (inTexture == nullptr) {
		return;
	}

	const float width = inTexture->getWidth();
	const float height = inTexture->getHeight();

	// Resize FBOs according to the input FBO
	if (width != fbo1.getWidth() || height != fbo1.getHeight()) {
		fbo1.clear();
		fbo2.clear();
		fbo1.allocate(width, height);
		fbo2.allocate(width, height);

		fbo1.begin();
		ofClear(0.f);
		fbo1.end();

		fbo2.begin();
		ofClear(0.f);
		fbo2.end();

		feedback_shader.begin();
		feedback_shader.setUniform2i("resolution", width, height);
		feedback_shader.end();
	}
}