#pragma once

#include "i-node.h"

#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	class Feedback : public INode {
	public:
		Feedback();
		~Feedback();

		void Update(UpdateParams* params) override;

		void Draw(DrawParams* params) override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		bool GuiDrawPropertiesList() override;

	private:
		bool ReloadShader();

		// Use two FBOs to ping pong for feedback.
		ofFbo fbo1;
		ofFbo fbo2;
		bool write_to_fbo1 = true;

		ofShader feedback_shader;

		GLuint bound_texture_id = 0;
		bool bound_texture_dirty = false;

		PinFbo<1> pin_in_texture = PinFbo<1>("input texture");
		PinFloat<1> pin_in_decay = PinFloat<1>("decay", "", { 0.05f }, 0.f, 1.f);
		PinFloat<4> pin_in_filter_color = PinFloat<4>("filter color", "", { 1.0f, 1.0f, 1.0f, 1.0f }, 0.f, 1.f);
		PinFloat<2> pin_in_feedback_offset = PinFloat<2>("feedback offset", "", { 0.f, 0.f });
			
		std::array<IPinInput*, 4> pin_inputs = {
			&pin_in_texture,
			&pin_in_decay,
			&pin_in_filter_color,
			&pin_in_feedback_offset,
		};

		PinOutput pin_out_texture = pins::SetupOutputPin(this, pins::PinType::FBO, "output");
	};
}