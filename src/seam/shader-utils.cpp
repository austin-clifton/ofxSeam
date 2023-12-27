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

	bool LoadShader(ofShader& shader, const std::string& vert_name, const std::string& frag_name, const std::string& geo_name) {
		if (shader.isLoaded()) {
			shader.unload();
		}

		// TODO should not need to prefix with the cwd in "normal" OF
		// something around loading from the bin path was broken with the c++17 update
		std::filesystem::path vert_path = std::filesystem::current_path() / "data/shaders" / vert_name;
		std::filesystem::path frag_path = std::filesystem::current_path() / "data/shaders" / frag_name;
		std::filesystem::path geo_path = geo_name.length() ? std::filesystem::current_path() / "data/shaders" / geo_name : "";
		
		// Make sure filepaths are valid; only check the geo path if the geo name has a length.
		if (!std::filesystem::exists(vert_path) 
			|| !std::filesystem::exists(frag_path)
			|| (geo_name.length() && !std::filesystem::exists(geo_path))
		) {
			return false;
		}
		
		if (!shader.load(vert_path, frag_path, geo_path)) {
			printf("failed to load shader at paths:\n\t%ls\t%ls\t%ls\n", vert_path.c_str(), frag_path.c_str(), geo_path.c_str());
			return false;
		} else {
			return true;
		}
	}
}