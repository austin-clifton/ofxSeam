#include "fireflies.h"

using namespace seam;
using namespace seam::nodes;

namespace {
	glm::vec3 PosPtr(glm::vec3* ptr) {
		return *ptr;
	}
}

Fireflies::Fireflies() : INode("Fireflies"), octree(glm::vec3(0.f), glm::vec3(100.f), PosPtr) {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);

	// TEMP!!! Until this actually receives time from something....
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);

	gui_display_fbo = &fbo;

	std::vector<glm::vec3> ff_directions;

	ff_positions.resize(fireflies_count);
	ff_directions.resize(fireflies_count, glm::vec3(0.f));

	// Randomize initial positions of fireflies
	for (size_t i = 0; i < ff_positions.size(); i++) {
		glm::vec3 dir = glm::normalize(glm::vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX) - 0.5f);
		float offset = (float)rand() / RAND_MAX * 90.0f;
		glm::vec3 pos = dir * offset + glm::vec3(0.001f);
		printf("%f %f %f\n", pos.x, pos.y, pos.z);
		ff_positions[i] = pos;
		octree.Add(&ff_positions[i], ff_positions[i]);
	}

	printf("%p\n", ff_positions.data());

	// Set up buffers
	glGenBuffers(1, &ff_positions_ssbo);
	glGenBuffers(1, &ff_directions_ssbo);
	
	// Set up immutable storage for positions and directions so we can persistently map and read/write stuff across the cpu/gpu barrier.
	
	// We're going to read the position buffer back to CPU for octree operations which influence particle directions,
	// hence using GL_MAP_READ_BIT here.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ff_positions_ssbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, ff_positions.size() * sizeof(glm::vec3),
		ff_positions.data(), GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// We'll write to the direction buffer after octree operations for avoidance behaviors.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ff_directions_ssbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, ff_directions.size() * sizeof(glm::vec3),
		ff_directions.data(), GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Map the SSBO position and direction buffers persistently for read and write respectively.
	ff_posmap = glMapNamedBufferRange(ff_positions_ssbo, 0, ff_positions.size() * sizeof(glm::vec3), GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT);
	ff_dirmap = glMapNamedBufferRange(ff_directions_ssbo, 0, ff_directions.size() * sizeof(glm::vec3), GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
	assert(ff_posmap != nullptr);
	assert(ff_dirmap != nullptr);

	ff_vertex_buffer.allocate(ff_positions, GL_DYNAMIC_DRAW);

	// Seed compute-only buffers.
	std::vector<ComputeFirefly> fireflies;
	fireflies.resize(fireflies_count);
	for (int i = 0; i < fireflies.size(); i++) {
		auto& ff = fireflies[i];
		ff.mass = 1.f;
		ff.luminance = 1.f;
		ff.effect_index = 0;
		ff.group_index = i;

		ff.velocity = glm::vec3(0.f);
		ff.acceleration = glm::vec3(0.f, 0.f, 0.f);

		ff.color = glm::vec4(float(i % 2), float((i + 1) % 2), 0.5, 1.0);
	}

	ff_compute_ssbo.allocate(fireflies, GL_DYNAMIC_DRAW);
	ff_compute_ssbo2.allocate(fireflies, GL_DYNAMIC_DRAW);

	// Set up the VBO
	ff_vbo.setVertexBuffer(ff_vertex_buffer, 3, 0); // TODO is stride = 0 right?
	ff_vbo.setColorBuffer(ff_compute_ssbo, sizeof(ComputeFirefly), sizeof(float) * 2 + sizeof(int) * 2);

	// TODO vbo.setAttributeBuffer() calls

	ff_vbo.enableColors();

	// TODO texture size as an input
	fbo.allocate(1920, 1080);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ff_positions_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ff_directions_ssbo);
	ff_vertex_buffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
	ff_compute_ssbo.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
	ff_compute_ssbo2.bindBase(GL_SHADER_STORAGE_BUFFER, 4);

	assert(ff_positions_ssbo != 0 && ff_directions_ssbo != 0 
		&& ff_vertex_buffer.getId() != 0 && ff_compute_ssbo.getId() != 0 && ff_compute_ssbo2.getId() != 0);

	LoadShaders();

	camera.setFov(90.f);

	box.setPosition(glm::vec3(0.f));
	box.setScale(glm::vec3(10.f));

	avoidance_thread = std::thread(&Fireflies::AvoidanceLoop, this);
}

Fireflies::~Fireflies() {
	stop = true;
	try {
		avoidance_thread.join();
	} catch (const std::system_error& e) {
		printf("Fireflies failed to join avoidance thread: %s\n", e.what());
	}

	glDeleteBuffers(1, &ff_positions_ssbo);
	glDeleteBuffers(1, &ff_directions_ssbo);
}

IPinInput** Fireflies::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* Fireflies::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_texture;
}

void Fireflies::Update(UpdateParams* params) {
	ff_compute_shader.begin();

	// TODO here set uniforms
	
	ff_compute_shader.dispatchCompute((fireflies_count + 1024 - 1) / 1024, 1, 1);
	ff_compute_shader.end();

	// Ping-pong copy written compute results back to the compute read buffer.
	ff_compute_ssbo.copyTo(ff_compute_ssbo2);
	// Also ping-pong copy positions back, these are stored separately since positions are mapped for CPU reading.
	glCopyNamedBufferSubData(ff_vertex_buffer.getId(), ff_positions_ssbo, 0, 0, sizeof(glm::vec3) * fireflies_count);

	if (invalidate_posmap) {
		invalidate_posmap = false;
		/*
		// Read back positions.
		glm::vec3* posmap = (glm::vec3*)ff_posmap;
		for (size_t i = 0; i < 10; i++) {
			printf("%f, %f, %f\n", posmap[i].x, posmap[i].y, posmap[i].z);
		}
		printf("\n");
		*/

		glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	}

	if (invalidate_dirmap) {
		invalidate_dirmap = false;
		glFlushMappedNamedBufferRange(ff_directions_ssbo, 0, sizeof(glm::vec3) * fireflies_count);
	}

}

void Fireflies::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(0.1f);
	ofEnableBlendMode(OF_BLENDMODE_ADD);

	camera.begin();
	ff_geo_shader.begin();

	glPointSize(2);
	ff_vbo.draw(GL_POINTS, 0, fireflies_count);

	ff_geo_shader.end();

	/*
	test_shader.begin();

	box.draw();

	test_shader.end();
	*/

	camera.end();
	fbo.end();
}

bool Fireflies::LoadShaders() {
	const std::filesystem::path shaders_path = std::filesystem::current_path() / "data" / "shaders";
	const std::string compute_name("fireflies-compute.glsl");
	const std::string vert_frag_name("fireflies");

	bool compute_loaded = ff_compute_shader.setupShaderFromFile(GL_COMPUTE_SHADER, shaders_path / compute_name)
		&& ff_compute_shader.linkProgram();

	ff_geo_shader.setGeometryInputType(GL_POINTS);
	ff_geo_shader.setGeometryOutputType(GL_TRIANGLE_STRIP);
	ff_geo_shader.setGeometryOutputCount(4);

	bool geo_loaded = ff_geo_shader.load(
		shaders_path / (vert_frag_name + ".vert"),
		shaders_path / (vert_frag_name + ".frag"),
		shaders_path / (vert_frag_name + ".geo")
	);

	bool test_loaded = test_shader.load(shaders_path / (vert_frag_name + ".vert"), shaders_path / (vert_frag_name + ".frag"));
	assert(test_loaded);

	if (!compute_loaded) {
		printf("Failed to load or link fireflies compute shader\n");
	}

	if (!geo_loaded) {
		printf("Failed to load or link fireflies geo/vert/frag shaders\n");
	}
	
	return compute_loaded && geo_loaded;
}

bool Fireflies::GuiDrawPropertiesList() {
	if (ImGui::Button("refresh")) {
		LoadShaders();
		return true;
	}
	return false;
}

void Fireflies::AvoidanceLoop() {
	assert(ff_posmap != nullptr && ff_dirmap != nullptr);
	glm::vec3* posmap = (glm::vec3*)ff_posmap;
	glm::vec3* dirmap = (glm::vec3*)ff_dirmap;
	std::vector<glm::vec3*> found;

	while (!stop) {
		// Request a refresh of mapped positions.
		invalidate_posmap = true;

		printf("%p\n", ff_positions.data());

		auto start = std::chrono::high_resolution_clock::now();

		// Update a segment of the avoidance direction map based on current positions.
		// For each particle, look nearby.
		for (size_t i = 0; i < fireflies_count; i++) {
			glm::vec3 dir(0.f);
			glm::vec3 my_pos = ff_positions[i];
			octree.FindItems(my_pos, pin_avoidance_radius[0], found);
			// Each nearby particle exerts a force based on how far away it is.
			// Closer particles exert more force.-
			for (size_t j = 0; j < found.size(); j++) {
				glm::vec3 their_pos = *found[j];
				glm::vec3 away_from_them = glm::normalize(my_pos - their_pos);
				float distance = glm::distance(my_pos, their_pos);
				// When almost at the edge, the force will be very low.
				float rev_dist01 = (pin_avoidance_radius[0] - distance) / pin_avoidance_radius[0];
				float force = pow(rev_dist01, 4);
				dir = dir + away_from_them * force;
			}
			dir = dir / std::max(1, (int)found.size());

			dirmap[i] = dir;
		}

		auto stop = std::chrono::high_resolution_clock::now();
		// printf("took %d ms to update directions\n", (stop - start).count() / 100000);
		
		// Ask the main thread to flush the segment of the direction buffer we just wrote to.
		invalidate_dirmap = true;

		start = std::chrono::high_resolution_clock::now();

		// Update the octree's positions.
		for (size_t i = 0; i < fireflies_count; i++) {
			glm::vec3 old_pos = ff_positions[i];
			glm::vec3 new_pos = posmap[i];
			octree.Update(&ff_positions[i], old_pos, new_pos);
			// Make sure to update the particle's position now so any re-organization the octree does can reference the new position.
			ff_positions[i] = new_pos;
		}

		// Copy the current posmap to the CPU's "old" positions list. 
		// memcpy_s(ff_positions.data(), ff_positions.size() * sizeof(glm::vec3), ff_posmap, fireflies_count * sizeof(glm::vec3));

		stop = std::chrono::high_resolution_clock::now();
		// printf("took %d ms to update positions\n", (stop - start).count() / 100000);

	}
}