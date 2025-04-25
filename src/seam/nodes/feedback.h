#pragma once

#include "seam/include.h"

using namespace seam::pins;

namespace seam::nodes {
	class Feedback : public INode {
	public:
		Feedback();

		void Setup(SetupParams* params) override;

		void Draw(DrawParams* params) override;

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		bool GuiDrawPropertiesList(UpdateParams* params) override;

	private:
		bool ReloadShader();
		void ResizeFrameBuffers();

		// Use two FBOs to ping pong for feedback.
		ofFbo fbo1;
		ofFbo fbo2;
		bool write_to_fbo1 = true;

		ofShader feedback_shader;

		ofFbo* inTexture = nullptr;
		float decay = 0.05f;
		glm::vec4 filterColor = glm::vec4(1.f);
		glm::vec2 feedbackOffset = glm::vec2(0.f);

		glm::vec2 radialCenter = glm::vec2(0.5f);
		float radialOffset = 2.0f;

		PinFloatMeta decayMeta = PinFloatMeta(0.f, 1.f);

		static const std::string fboInputName;
			
		std::array<PinInput, 6> pin_inputs = {
			pins::SetupInputFboPin(PinType::FboRgba, this, &feedback_shader, &inTexture, "imgTex", fboInputName,
				PinInOptions::WithChangedCallbacks([this]() { ResizeFrameBuffers(); })
			),
			pins::SetupInputPin(PinType::Float, this, &decay, 1, "Decay", PinInOptions("", &decayMeta)),
			pins::SetupInputPin(PinType::Float, this, &filterColor, 1, "Filter Color", PinInOptions::WithCoords(4)),
			pins::SetupInputPin(PinType::Float, this, &feedbackOffset, 1, "Feedback Offset", PinInOptions::WithCoords(2)),
			pins::SetupInputPin(PinType::Float, this, &radialCenter, 1, "Radial Center", PinInOptions::WithCoords(2)),
			pins::SetupInputPin(PinType::Float, this, &radialOffset, 1, "Radial Offset"),
		};

		PinOutput pinOutFbo = pins::SetupOutputPin(this, pins::PinType::FboRgba, "output");
	};
}