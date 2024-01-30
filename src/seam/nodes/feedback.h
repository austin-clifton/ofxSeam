#pragma once

#include "seam/nodes/iNode.h"
#include "seam/pins/pin.h"

using namespace seam::pins;

namespace seam::nodes {
	class Feedback : public INode {
	public:
		Feedback();
		~Feedback();

		void Setup(SetupParams* params) override;

		void Draw(DrawParams* params) override;

		void Update(UpdateParams* params) override;

		void OnPinConnected(PinConnectedArgs args) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

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
			pins::SetupInputPin(PinType::FBO_RGBA, this, &inTexture, 1, "Input FBO", 
				PinInOptions(std::bind(&Feedback::RebindTexture, this))),
			pins::SetupInputPin(PinType::FLOAT, this, &decay, 1, "Decay", PinInOptions("", &decayMeta)),
			pins::SetupInputPin(PinType::FLOAT, this, &filterColor, 1, "Filter Color", PinInOptions::WithCoords(4)),
			pins::SetupInputPin(PinType::FLOAT, this, &feedbackOffset, 1, "Feedback Offset", PinInOptions::WithCoords(2)),
		};

		PinOutput pinOutFbo = pins::SetupOutputPin(this, pins::PinType::FBO_RGBA, "output");
	};
}