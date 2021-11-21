#include "texgen-perlin.h"

using namespace seam;

namespace {
	// TODO move this
	void GuiDrawTexture() {

	}
}

TexgenPerlin::TexgenPerlin() : IEventNode("Perlin Noise") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// define input pins and their callbacks
	pin_inputs[0] = CreatePinInput<int>("octaves", [&](int v) {
		assert(octaves > 0);
		octaves = v;
		dirty = true;
	});

	pin_inputs[1] = CreatePinInput<float>("frequency", [&](float v) {
		frequency = v;
		dirty = true;
	});

	pin_inputs[2] = CreatePinInput<float>("lacunarity", [&](float v) {
		lacunarity = v;
		dirty = true;
	});

	pin_inputs[3] = CreatePinInput<float>("amplitude", [&](float v) {
		amplitude = v;
		dirty = true;
	});

	pin_inputs[4] = CreatePinInput<float>("persistence", [&](float v) {
		persistence = v;
		dirty = true;
	});

	// define the output pin
	pin_out_tex.pin.type = PinType::TEXTURE;
	pin_out_tex.pin.name = "texture";

	// TODO this shouldn't be an RGB buffer if only using one color channel
	fbo.allocate(tex_size.x, tex_size.y, GL_RGB);

	// TODO should not need to prefix with the cwd in "normal" OF
	// something around loading from the bin path was broken with the c++17 update
	std::filesystem::path shader_path = std::filesystem::current_path() / "data/shaders" / shader_name;
	if (!shader.load(shader_path)) {
		std::string strpath = shader_path.string();
		printf("failed to load shader at path %s\n", strpath.c_str());
	} else {
		// tell the shader what the image resolution is
		shader.setUniform2iv("resolution", &tex_size[0]);
	}

}

TexgenPerlin::~TexgenPerlin() {
	// delete input pins
	for (size_t i = 0; i < pin_inputs.size(); i++) {
		delete pin_inputs[i].pin;
		pin_inputs[i].pin = nullptr;
	}
}

PinInput* TexgenPerlin::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* TexgenPerlin::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_tex;
}

void TexgenPerlin::GuiDrawNodeView() {


	// TODO draw imgui texture
}

void TexgenPerlin::GuiDrawPropertiesList() {
	// TODO...
}