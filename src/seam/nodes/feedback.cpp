#include "feedback.h"
#include "../shader-utils.h"

using namespace seam;
using namespace seam::nodes;

Feedback::Feedback() : INode("Feedback") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);
	gui_display_fbo = &fbo1;

	// TODO texture size as a control of the input texture
	fbo1.allocate(1920, 1080, GL_RGBA);
	fbo2.allocate(1920, 1080, GL_RGBA);

	fbo1.begin();
	ofClear(0.f);
	fbo1.end();

	fbo2.begin();
	ofClear(0.f);
	fbo2.end();

	ReloadShader();
}

Feedback::~Feedback() {

}

void Feedback::Update(UpdateParams* params) {
	ofFbo* fbo = pin_in_texture[0];
	if (fbo != nullptr && bound_texture_id != fbo->getId()) {
		bound_texture_id = fbo->getId();
		bound_texture_dirty = true;
	}
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

	if (bound_texture_dirty && pin_in_texture[0] != nullptr) {
		feedback_shader.setUniformTexture("imgTex", pin_in_texture[0]->getTexture(), 1);
	}

	// feedback_shader.setUniformTexture("feedbackTex", fbo_read->getTexture(), 0);
	feedback_shader.setUniform1f("decay", pin_in_decay[0]);
	feedback_shader.setUniform4f("filterColor", pin_in_filter_color[0], pin_in_filter_color[1], 
		pin_in_filter_color[2], pin_in_filter_color[3]);
	feedback_shader.setUniform2f("feedbackOffset", pin_in_feedback_offset[0], pin_in_feedback_offset[1]);

	fbo_read->draw(0, 0);

	feedback_shader.end();
	fbo_write->end();

	write_to_fbo1 = !write_to_fbo1;
}

IPinInput** Feedback::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* Feedback::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_texture;
}

bool Feedback::GuiDrawPropertiesList() {
	if (ImGui::Button("Reload Shaders")) {
		return ReloadShader();
	}

	return false;
}

bool Feedback::ReloadShader() {
	return ShaderUtils::LoadShader(feedback_shader, "screen-rect.vert", "feedback.frag");
}