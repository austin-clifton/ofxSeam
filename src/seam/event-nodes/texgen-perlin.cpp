#include "texgen-perlin.h"
#include "../imgui-utils/properties.h"

using namespace seam;

namespace {
	// TODO move these
	void GuiDrawTexture() {

	}

	bool LoadShader(ofShader& shader, const std::string& shader_name) {
		if (shader.isLoaded()) {
			shader.unload();
		}

		// TODO should not need to prefix with the cwd in "normal" OF
		// something around loading from the bin path was broken with the c++17 update
		std::filesystem::path shader_path = std::filesystem::current_path() / "data/shaders" / shader_name;
		if (!shader.load(shader_path)) {
			std::string strpath = shader_path.string();
			printf("failed to load shader at path %s\n", strpath.c_str());
			return false;
		} else {
			return true;
		}
	}
}

TexgenPerlin::TexgenPerlin() : IEventNode("Perlin Noise") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// define the output pin
	pin_out_tex.pin = SetupPinOutput(PinType::TEXTURE, "texture");

	// TODO this shouldn't be an RGB buffer if only using one color channel
	ofFbo::Settings settings;
	settings.width = tex_size.x;
	settings.height = tex_size.y;
	settings.internalformat = GL_RGB;
	settings.textureTarget = GL_TEXTURE_2D;
	fbo.allocate(settings);
	// fbo.allocate(tex_size.x, tex_size.y, GL_RGB);

	if (LoadShader(shader, shader_name)) {
		// tell the shader what the image resolution is
		shader.setUniform2iv("resolution", &tex_size[0]);
	}
}

TexgenPerlin::~TexgenPerlin() {

}

void TexgenPerlin::Draw() {
	fbo.begin();
	shader.begin();

	shader.setUniform1i("octaves", pin_octaves.value);
	shader.setUniform1f("frequency", pin_frequency.value);
	shader.setUniform1f("lacunarity", pin_lacunarity.value);
	shader.setUniform1f("amplitude", pin_amplitude.value);
	shader.setUniform1f("persistence", pin_persistence.value);

	fbo.draw(0, 0);

	shader.end();
	fbo.end();
}

void TexgenPerlin::DrawToScreen() {
	fbo.draw(0, 0);
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
	ImVec2 wsize = ImGui::GetContentRegionAvail();
	// oof
	// https://forum.openframeworks.cc/t/how-to-draw-an-offbo-in-ofximgui-window/33174/2
	ImTextureID texture_id = (ImTextureID)(uintptr_t)fbo.getTexture().getTextureData().textureID;
	ImGui::Image(texture_id, ImVec2(256, 256));



	// TODO draw imgui texture
}

bool TexgenPerlin::GuiDrawPropertiesList() {
	if (props::Draw("shader name", shader_name, true)) {
		if (LoadShader(shader, shader_name)) {
			// tell the shader what the image resolution is
			shader.setUniform2iv("resolution", &tex_size[0]);
			printf("reloaded shader\n");

			return true;
		}
	}
	return false;
}