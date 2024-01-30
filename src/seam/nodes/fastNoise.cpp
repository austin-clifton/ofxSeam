#include "seam/nodes/fastNoise.h"
#include "seam/shaderUtils.h"

using namespace seam::nodes;

FastNoise::FastNoise() : INode("Fast Noise") {
    // TEMP: always use UPDATES_OVER_TIME, but there should be a way to flag and unflag
    // nodes for over-time updates instead.
    flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL | NodeFlags::UPDATES_OVER_TIME);
    gui_display_fbo = &fbo;

    // Input pins are set up in the constructor since they require a loop.

    std::array<PinInput, 15> nonEnumInputPins = {
        SetupInputPin(PinType::INT, this, &resolution, 1, "Resolution", 
            PinInOptions(sizeof(glm::vec2), 2, 0, std::bind(&FastNoise::ResizeFbo, this))),
        SetupInputPin(PinType::INT, this, &vSplits, 1, "Vertical Splits"),
        SetupInputPin(PinType::INT, this, &hSplits, 1, "Horizontal Splits"),
        SetupInputPin(PinType::FLOAT, this, &speed, 1, "Speed"),
        SetupInputPin(PinType::BOOL, this, &animateOverTime, 1, "Animate Over Time"),
        SetupInputPin(PinType::BOOL, this, &enableDomainWarp, 1, "Enable Domain Warp"),
        SetupInputPin(PinType::INT, this, &seed, 1, "Seed"),
        SetupInputPin(PinType::FLOAT, this, &frequency, 1, "Frequency"),
        SetupInputPin(PinType::INT, this, &octaves, 1, "Octaves"),
        SetupInputPin(PinType::FLOAT, this, &lacunarity, 1, "Lacunarity"),
        SetupInputPin(PinType::FLOAT, this, &gain, 1, "Gain"),
        SetupInputPin(PinType::FLOAT, this, &weightedStrength, 1, "Weighted Strength"),
        SetupInputPin(PinType::FLOAT, this, &pingPongStrength, 1, "Ping Pong Strength"),
        SetupInputPin(PinType::FLOAT, this, &cellularJitterMod, 1, "Cellular Jitter Mod"),
        SetupInputPin(PinType::FLOAT, this, &domainWarpAmp, 1, "Domain Warp Amp"),
    };

    // Exactly this many non-enum pins are expected!
    assert(pinInputs.size() - fnlOptions.size() == nonEnumInputPins.size());

    std::move(nonEnumInputPins.begin(), nonEnumInputPins.end(), pinInputs.begin());

    for (size_t i = nonEnumInputPins.size(), j = 0; i < pinInputs.size(); i++, j++) {
        pinInputs[i] = SetupInputPin(PinType::INT, this, &fnlOptions[j].currValue, 1, 
            fnlOptions[j].optionName, PinInOptions("", &fnlOptions[j].meta));
    }
}

void FastNoise::Setup(SetupParams* params) {
    ResizeFbo();
    ReloadShaders();

}

void FastNoise::Draw(DrawParams* params) {
    // Set uniforms; ideally this should be done in less GPU calls???
    // Each pin could have a callback, but that might be more expensive with binding/unbinding the shader.
    // A cpu-writable SSBO might be more ideal for flushing info over, idk
    shader.begin();

    // Advance time if time advancement is enabled.
    internalTime = internalTime + animateOverTime * params->delta_time * speed;
    shader.setUniform1f("time", internalTime);

    shader.setUniform2i("resolution", resolution.x, resolution.y);
    shader.setUniform1i("vSplits", vSplits);
    shader.setUniform1i("hSplits", hSplits);

    shader.setUniform1i("seed", seed);
    shader.setUniform1f("frequency", frequency);
    shader.setUniform1i("octaves", octaves);
    shader.setUniform1f("lacunarity", lacunarity);
    shader.setUniform1f("gain", gain);
    shader.setUniform1f("weightedStrength", weightedStrength);
    shader.setUniform1f("pingPongStrength", pingPongStrength);
    shader.setUniform1f("cellularJitterMod", cellularJitterMod);
    shader.setUniform1f("domainWarpAmp", domainWarpAmp);
    shader.setUniform1i("enableDomainWarp", enableDomainWarp);

    for (size_t i = 0; i < fnlOptions.size(); i++) {
        shader.setUniform1i(fnlOptions[i].uniformName, fnlOptions[i].currValue);
    }

    fbo.begin();
    fbo.draw(0,0);
    fbo.end();

    shader.end();

    pinOutFbo.DirtyConnections();
}

bool FastNoise::GuiDrawPropertiesList(UpdateParams* params) {
    bool changed = false;
    for (size_t i = 0; i < fnlOptions.size(); i++) {
        FnlOption& op = fnlOptions[i];
        if (ImGui::BeginCombo(op.optionName.data(), op.valueNames[op.currValue])) {
            for (size_t j = 0; j <= (size_t)op.maxValue; j++) {
                bool selected = j == (size_t)op.currValue;
                if (ImGui::Selectable(op.valueNames[j], &selected)) {
                    changed = true;
                    op.currValue = j;
                }
            }
            ImGui::EndCombo();
        }
    }

    if (ImGui::Button("Reload Shader")) {
        changed = ReloadShaders() || changed;
    }

    return changed;
}

PinInput* FastNoise::PinInputs(size_t& size) {
    size = pinInputs.size();
    return pinInputs.data();
}

PinOutput* FastNoise::PinOutputs(size_t& size) {
    size = 1;
    return &pinOutFbo;
}

void FastNoise::ResizeFbo() {
    fbo.clear();
    fbo.allocate(resolution.x, resolution.y, GL_RGB);

    fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
}

bool FastNoise::ReloadShaders() {
    const std::string fragName = "fastNoiseLite.frag";
    return ShaderUtils::LoadShader(shader, "screen-rect.vert", fragName);
}

void FastNoise::OnPinConnected(PinConnectedArgs args) {
	// The output FBO doesn't change; only push it on pin connected.
	if (args.pinOut->id == pinOutFbo.id) {
		ofFbo* fbos = { &fbo };
		args.pushPatterns->Push<ofFbo*>(pinOutFbo, &fbos, 1);
	}
}