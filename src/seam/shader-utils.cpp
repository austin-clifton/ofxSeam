#include "shader-utils.h"

namespace seam::ShaderUtils {
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