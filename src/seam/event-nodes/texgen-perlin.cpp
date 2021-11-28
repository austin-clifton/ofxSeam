#include "texgen-perlin.h"

using namespace seam;

namespace {
	// TODO move this
	void GuiDrawTexture() {

	}
}

TexgenPerlin::TexgenPerlin() : IEventNode("Perlin Noise") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// define the output pin
	pin_out_tex.pin = SetupPinOutput(PinType::TEXTURE, "texture");

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