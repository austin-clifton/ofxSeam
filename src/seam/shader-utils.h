#pragma once

#include "ofMain.h"

namespace seam::ShaderUtils {
	/// load or reload a shader
	bool LoadShader(ofShader& shader, const std::string& shader_name);
}