#pragma once

#include "ofMain.h"

namespace seam::ShaderUtils {
	/// load or reload a shader
	bool LoadShader(ofShader& shader, const std::string& shader_name);
	bool LoadShader(ofShader& shader, const std::string& vert_name, const std::string& frag_name, const std::string& geo_name = "");
}