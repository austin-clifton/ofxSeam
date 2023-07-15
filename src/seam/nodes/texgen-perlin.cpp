#include "texgen-perlin.h"
#include "../imgui-utils/properties.h"
#include "../shader-utils.h"

using namespace seam;
using namespace seam::nodes;

TexgenPerlin::TexgenPerlin() : INode("Perlin Noise") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// TODO this shouldn't be an RGB buffer if only using one color channel
	ofFbo::Settings settings;
	settings.width = tex_size.x;
	settings.height = tex_size.y;
	settings.internalformat = GL_RGB;
	settings.textureTarget = GL_TEXTURE_2D;
	fbo.allocate(settings);
	// fbo.allocate(tex_size.x, tex_size.y, GL_RGB);

	if (ShaderUtils::LoadShader(shader, shader_name)) {
		// tell the shader what the image resolution is
		shader.setUniform2iv("resolution", &tex_size[0]);
	}

	gui_display_fbo = &fbo;
}

TexgenPerlin::~TexgenPerlin() {

}

void TexgenPerlin::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(0.f);
	shader.begin();

	shader.setUniform1i("octaves", pin_octaves[0]);
	shader.setUniform1f("frequency", pin_frequency[0]);
	shader.setUniform1f("lacunarity", pin_lacunarity[0]);
	shader.setUniform1f("amplitude", pin_amplitude[0]);
	shader.setUniform1f("persistence", pin_persistence[0]);

	fbo.draw(0, 0);

	shader.end();
	fbo.end();
}

IPinInput** TexgenPerlin::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* TexgenPerlin::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_tex;
}

/*
bool TexgenPerlin::GuiDrawPropertiesList() {
	if ( props::DrawShaderPath("shader name", shader_name) 
		&& ShaderUtils::LoadShader(shader, shader_name)
	) {
		// reloaded, tell the shader what the image resolution is again
		shader.setUniform2iv("resolution", &tex_size[0]);
		return true;
	}
	return false;
}
*/