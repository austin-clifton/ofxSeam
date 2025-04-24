#include "seam/nodes/valueNoise.h"
#include "seam/imguiUtils/properties.h"
#include "seam/shaderUtils.h"

using namespace seam;
using namespace seam::nodes;

ValueNoise::ValueNoise() : INode("Value Noise") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);
	gui_display_fbo = &fbo;
}

void ValueNoise::Setup(SetupParams* params) {
	ResizeFbo();
	ReloadShader();
}

void ValueNoise::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(0.f);
	shader.begin();

	uniformsPinMap.SetUniforms();

	/*
	shader.setUniform1i("octaves", octaves);
	shader.setUniform1f("frequency", frequency);
	shader.setUniform1f("lacunarity", lacunarity);
	shader.setUniform1f("seed", seed);
	*/
	shader.setUniform2i("resolution", resolution.x, resolution.y);

	fbo.draw(0, 0);

	shader.end();
	fbo.end();
}

PinInput* ValueNoise::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

PinOutput* ValueNoise::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutFbo;
}

bool ValueNoise::GuiDrawPropertiesList(UpdateParams* params) {
	bool changed = INode::GuiDrawPropertiesList(params);

	return (ImGui::Button("Reload Shader") && ReloadShader()) || changed;
}

std::vector<props::NodeProperty> ValueNoise::GetProperties() {
	using namespace seam::props;

	std::vector<NodeProperty> props;
	props.push_back(SetupUintProperty("Resolution", [this](size_t& size) {
		size = 2;
		return (uint32_t*)&resolution;
	}, [this](uint32_t* newRes, size_t size) {
		assert(size == 2);
		resolution = (glm::uvec2)(*newRes);
	}));

	return props;
}

bool ValueNoise::ReloadShader() {
	if (ShaderUtils::LoadShader(shader, "screen-rect.vert", "valueNoise.frag")) {
		uniformsPinMap.UpdatePins();
		return true;
	}
	return false;
}

void ValueNoise::ResizeFbo() {
	fbo.clear();
	fbo.allocate(resolution.x, resolution.y, GL_RGBA16F);
}