#include "seam/nodes/shader.h"
#include "seam/imguiUtils/properties.h"
#include "seam/shaderUtils.h"

using namespace seam;
using namespace seam::nodes;

// TODO break this out so it can be used by any Nodes that want it
namespace {
	const std::array<std::pair<const char*, int>, 4> textureWrapModes = {
		std::make_pair("Repeat", GL_REPEAT),
		std::make_pair("Mirror Repeat", GL_MIRRORED_REPEAT),
		std::make_pair("Clamp to Edge", GL_CLAMP_TO_EDGE),
		std::make_pair("Clamp to Border", GL_CLAMP_TO_BORDER),
	};

	const char* GlTextureWrapModeToName(int wrapMode) {
		auto it = std::find_if(textureWrapModes.begin(), textureWrapModes.end(), 
			[wrapMode](const std::pair<const char*, int>& pair) {
				return wrapMode == pair.second;
			}
		);
		assert(it != textureWrapModes.end());
		return it != textureWrapModes.end() ? it->first : textureWrapModes[0].first;
	}

	bool DrawImGuiTextureWrapCombo(const char* name, int& value) {
		bool changed = false;
		if (ImGui::BeginCombo(name, GlTextureWrapModeToName(value))) {
			for (auto pair : textureWrapModes) {
				bool selected = value == pair.second;
				if (ImGui::Selectable(pair.first, &selected)) {
					changed = true;
					value = pair.second;
				}
			}
			ImGui::EndCombo();
		}
		return changed;
	}
}

Shader::Shader() : INode("Shader Material") {
	flags = (NodeFlags)(flags | NodeFlags::IsVisual);

	windowFbos.push_back(WindowRatioFbo(&fbo, &pinOutMaterial));
	gui_display_fbo = &fbo;
}

void Shader::Setup(SetupParams* params) {
	AttemptShaderLoad(shader_name);
}

Shader::~Shader() {
	// TODO dealloc pin input names
}

PinInput* Shader::PinInputs(size_t& size) {
	size = pinInputs.size();
	return pinInputs.data();
}

PinOutput* Shader::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutMaterial;
}

void Shader::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(ofFloatColor(0.0f));
	shader.begin();

	uniformsPin.SetUniforms();

	fbo.draw(0, 0);

	shader.end();
	fbo.end();
}

bool Shader::AttemptShaderLoad(const std::string& shader_name) {
	if (ShaderUtils::LoadShader(shader, "screen-rect.vert", shader_name + ".frag")) {
		uniformsPin.UpdatePins();
		return true;
	}
	return false;
}

void Shader::OnFilteringChanged() {
	int filterType = filterLinear ? GL_LINEAR : GL_NEAREST;
	fbo.getTexture().setTextureMinMagFilter(filterType, filterType);
}

void Shader::OnTextureWrapChanged() {
	fbo.getTexture().setTextureWrap(textureWrapHorizontal, textureWrapVertical);
}

bool Shader::GuiDrawPropertiesList(UpdateParams* params) {
	bool shaderChanged = false;
	bool texWrapChanged = false;

	// Ask the GUI to draw an input for the shader path.
	if (props::DrawShaderPath("Shader Name", shader_name)) {
		shaderChanged = AttemptShaderLoad(shader_name);
	}

	texWrapChanged = DrawImGuiTextureWrapCombo(
		"Texture Wrap Horizontal", 
		textureWrapHorizontal
	);

	texWrapChanged = DrawImGuiTextureWrapCombo(
		"Texture Wrap Vertical", 
		textureWrapVertical
	) || texWrapChanged;

	if (texWrapChanged) {
		OnTextureWrapChanged();
	}

	return shaderChanged || texWrapChanged;
}

std::vector<props::NodeProperty> Shader::GetProperties() {
	std::vector<props::NodeProperty> properties;
	
	properties.push_back(props::SetupStringProperty("Frag Shader Name", [this](size_t& size) {
		size = 1;
		return &shader_name;
	}, [this](std::string* newName, size_t size) {
		assert(size == 1);
		shader_name = *newName;
		AttemptShaderLoad(shader_name);
	}));

	// TODO break out texture wrap stuff so it can be used by other Nodes...
	properties.push_back(props::SetupIntProperty("Texture Wrap Horizontal", [this](size_t& size) {
		size = 1;
		return &textureWrapHorizontal;
	}, [this](int* newTextureWrap, size_t size) {
		assert(size == 1);
		textureWrapHorizontal = *newTextureWrap;
		OnTextureWrapChanged();
	}));

	properties.push_back(props::SetupIntProperty("Texture Wrap Vertical", [this](size_t& size) {
		size = 1;
		return &textureWrapVertical;
	}, [this](int* newTextureWrap, size_t size) {
		assert(size == 1);
		textureWrapVertical = *newTextureWrap;
		OnTextureWrapChanged();
	}));

	return properties;
}