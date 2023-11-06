#pragma once

#include "iNode.h"

#include "../pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	class Feedback : public INode {
	public:
		Feedback();
		~Feedback();

		void Draw(DrawParams* params) override;

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		bool GuiDrawPropertiesList() override;

	private:
		bool ReloadShader();
		void RebindTexture();

		// Use two FBOs to ping pong for feedback.
		ofFbo fbo1;
		ofFbo fbo2;
		bool write_to_fbo1 = true;

		ofShader feedback_shader;

		ofFbo* inTexture = nullptr;
		float decay = 0.05f;
		glm::vec4 filterColor = glm::vec4(1.f);
		glm::vec2 feedbackOffset = glm::vec2(0.f);

		PinFloatMeta decayMeta = PinFloatMeta(0.f, 1.f);
			
		std::array<PinInput, 4> pin_inputs = {
			pins::SetupInputPin(PinType::FBO, this, &inTexture, 1, "Input FBO", 
				PinInOptions(std::bind(&Feedback::RebindTexture, this))),
			pins::SetupInputPin(PinType::FLOAT, this, &decay, 1, "Decay", PinInOptions("", &decayMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &filterColor, 4, "Filter Color"),
			pins::SetupInputPin(PinType::FLOAT, this, &feedbackOffset, 2, "Feedback Offset"),
		};

		PinOutput pin_out_texture = pins::SetupOutputPin(this, pins::PinType::FBO, "output");
	};
}