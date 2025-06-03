#include "seam/nodes/fastNoise.h"
#include "seam/shaderUtils.h"
#include "seam/pins/pin.h"

using namespace seam::nodes;

FastNoise::FastNoise() : INode("Fast Noise") {
    flags = (NodeFlags)(flags | NodeFlags::IsVisual | NodeFlags::UpdatesOverTime);
    gui_display_fbo = &fbo;

    // Input pins are set up in the constructor since they require a loop.

    std::array<PinInput, 18> nonEnumInputPins = {
        SetupInputPin(PinType::Int, this, &resolution, 1, "Resolution", 
            PinInOptions(sizeof(glm::vec2), 2, 0, std::bind(&FastNoise::ResizeFbo, this))),
        SetupInputPin(PinType::Int, this, &vSplits, 1, "Vertical Splits"),
        SetupInputPin(PinType::Int, this, &hSplits, 1, "Horizontal Splits"),
        SetupInputPin(PinType::Float, this, &speed, 1, "Speed"),
        SetupInputPin(PinType::Bool, this, &animateOverTime, 1, "Animate Over Time",
            PinInOptions::WithChangedCallbacks([this]() {
                SetUpdatesOverTime(animateOverTime);
            }
        )),
        SetupInputPin(PinType::Bool, this, &enableDomainWarp, 1, "Enable Domain Warp"),
        SetupInputPin(PinType::Bool, this, &filterNearest, 1, "Filter Nearest"),
        SetupInputPin(PinType::Int, this, &seed, 1, "Seed"),
        SetupInputPin(PinType::Float, this, &frequency, 1, "Frequency"),
        SetupInputPin(PinType::Int, this, &octaves, 1, "Octaves"),
        SetupInputPin(PinType::Float, this, &lacunarity, 1, "Lacunarity"),
        SetupInputPin(PinType::Float, this, &gain, 1, "Gain"),
        SetupInputPin(PinType::Float, this, &weightedStrength, 1, "Weighted Strength"),
        SetupInputPin(PinType::Float, this, &pingPongStrength, 1, "Ping Pong Strength"),
        SetupInputPin(PinType::Float, this, &cellularJitterMod, 1, "Cellular Jitter Mod"),
        SetupInputPin(PinType::Float, this, &domainWarpAmp, 1, "Domain Warp Amp"),
        SetupInputPin(PinType::Bool, this, &calculateGChannel, 1, "Calculate G Channel"),
        SetupInputPin(PinType::Bool, this, &calculateBChannel, 1, "Calculate B Channel"),
    };

    // Exactly this many non-enum pins are expected!
    assert(pinInputs.size() - fnlOptions.size() == nonEnumInputPins.size());

    std::move(nonEnumInputPins.begin(), nonEnumInputPins.end(), pinInputs.begin());

    for (size_t i = nonEnumInputPins.size(), j = 0; i < pinInputs.size(); i++, j++) {
        pinInputs[i] = SetupInputPin(PinType::Int, this, &fnlOptions[j].currValue, 1, 
            fnlOptions[j].optionName, PinInOptions("", &fnlOptions[j].meta));
    }
}

void FastNoise::Setup(SetupParams* params) {
    ResizeFbo();
    ReloadShaders();

}

void FastNoise::Draw(DrawParams* params) {
    if (!animateOverTime && !IsDirty()) {
        return;
    }

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
    shader.setUniform1i("calculateG", (int)calculateGChannel);
    shader.setUniform1i("calculateB", (int)calculateBChannel);

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

    if (filterNearest) {
        fbo.getTexture().setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
    } else {
        fbo.getTexture().setTextureMinMagFilter(GL_LINEAR, GL_LINEAR);
    }
}

bool FastNoise::ReloadShaders() {
    const std::string fragName = "fastNoiseLite.frag";
    return ShaderUtils::LoadShader(shader, "screen-rect.vert", fragName);
}