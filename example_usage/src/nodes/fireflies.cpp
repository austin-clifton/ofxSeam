#include "fireflies.h"

#include "seam/pins/push.h"

using namespace seam;
using namespace seam::nodes;

namespace {
	glm::vec4 PosPtr(glm::vec4* ptr) {
		return *ptr;
	}
}

Fireflies::Fireflies() : INode("Fireflies"), 
	ff_octree(glm::vec3(0.f), glm::vec3(512.f), PosPtr),  
	mom_octree(glm::vec3(0.f), glm::vec3(512.f), PosPtr)
{
	flags = (NodeFlags)(flags | NodeFlags::IsVisual);

	// TEMP!!! Until this actually receives time from something....
	flags = (NodeFlags)(flags | NodeFlags::UpdatesOverTime);

	gui_display_fbo = &fbo;
	windowFbos.push_back(WindowRatioFbo(&fbo, &pinOutFbo));
}

void Fireflies::Setup(SetupParams* params) {
	// Set up initial directions for fireflies and their moms.
	// Declare these inline and not as members since the CPU writes directions to mapped buffers later;
	// these are only needed for initialization.
	std::vector<glm::vec4> ff_directions;
	std::vector<glm::vec4> mom_directions;

	ff_positions.resize(fireflies_count, glm::vec4(0.f, 0.f, 0.f, 1.0f));
	ff_directions.resize(fireflies_count, glm::vec4(0.f));
	mom_positions.resize(moms_count, glm::vec4(0.f, 0.f, 0.f, 1.f));
	mom_directions.resize(moms_count, glm::vec4(0.f));

	// Randomize initial positions of mooooms
	for (size_t i = 0; i < mom_positions.size(); i++) {
		glm::vec3 pos = glm::vec3(
			ofRandomf() * 256.f,
			ofRandomf() * 10.f + 10.f,
			ofRandomf() * 256.f
		);
		mom_positions[i] = glm::vec4(pos, 1.f);
		mom_octree.Add(&mom_positions[i], mom_positions[i]);
	}

	// Randomize initial positions of fireflies
	for (size_t i = 0; i < ff_positions.size(); i++) {
		glm::vec3 dir = glm::normalize(glm::vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX) - 0.5f);
		float offset = (float)rand() / RAND_MAX * 90.0f;
		glm::vec3 pos = dir * offset + glm::vec3(0.001f);
		ff_positions[i] = glm::vec4(pos, 1.f);
		ff_octree.Add(&ff_positions[i], ff_positions[i]);
	}

	// Set up buffers
	glGenBuffers(1, &ff_positions_ssbo);
	glGenBuffers(1, &ff_directions_ssbo);
	glGenBuffers(1, &mom_positions_ssbo);
	glGenBuffers(1, &mom_directions_ssbo);
	
	// Set up immutable storage for positions and directions so we can persistently map and read/write stuff across the cpu/gpu barrier.
	
	// We're going to read the position buffer back to CPU for octree operations which influence particle directions,
	// hence using GL_MAP_READ_BIT here.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ff_positions_ssbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, ff_positions.size() * sizeof(glm::vec4),
		ff_positions.data(), GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// We'll write to the direction buffer after octree operations for avoidance behaviors.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ff_directions_ssbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, ff_directions.size() * sizeof(glm::vec4),
		ff_directions.data(), GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Do the same thing for moms now.
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mom_positions_ssbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, mom_positions.size() * sizeof(glm::vec4),
		mom_positions.data(), GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, mom_directions_ssbo);
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, mom_directions.size() * sizeof(glm::vec4),
		mom_directions.data(), GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	// Map the SSBO position and direction buffers persistently for read and write respectively.
	ff_posmap = (glm::vec4*)glMapNamedBufferRange(ff_positions_ssbo, 0, ff_positions.size() * sizeof(glm::vec4), GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT);
	ff_dirmap = (glm::vec4*)glMapNamedBufferRange(ff_directions_ssbo, 0, ff_directions.size() * sizeof(glm::vec4), GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
	mom_posmap = (glm::vec4*)glMapNamedBufferRange(mom_positions_ssbo, 0, mom_positions.size() * sizeof(glm::vec4), GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT);
	mom_dirmap = (glm::vec4*)glMapNamedBufferRange(mom_directions_ssbo, 0, mom_directions.size() * sizeof(glm::vec4), GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT);
	assert(ff_posmap != nullptr);
	assert(ff_dirmap != nullptr);
	assert(mom_posmap != nullptr);
	assert(mom_dirmap != nullptr);

	ff_vertex_buffer.allocate(ff_positions, GL_DYNAMIC_DRAW);
	mom_vertex_buffer.allocate(mom_positions, GL_DYNAMIC_DRAW);

	// Seed compute-only buffers.
	std::vector<ComputeMomma> moms;
	moms.resize(moms_count);
	for (size_t i = 0; i < moms.size(); i++) {
		auto& mom = moms[i];
		mom.mass = 1.f;
		mom.velocity = glm::vec3(0.f);

		mom.col_a = glm::vec3(0.5f,0.25f,0.5f);
		mom.col_b = glm::vec3(0.5f,0.25f,0.5f);
		mom.col_c = glm::vec3(1.0f);;
		mom.col_d = glm::vec3(0.f, 0.33f, 0.667f);
	}

	std::vector<ComputeFirefly> fireflies;
	fireflies.resize(fireflies_count);
	for (size_t i = 0; i < fireflies.size(); i++) {
		auto& ff = fireflies[i];
		ff.mass = 1.f;
		ff.luminance = 1.f;
		ff.effect_index = 0;
		ff.group_index = i;

		ff.velocity = glm::vec3(0.f);

		ff.color = glm::vec4(1.0);
	}

	ff_compute_ssbo_read.allocate(fireflies, GL_DYNAMIC_DRAW);
	ff_compute_ssbo_write.allocate(fireflies, GL_DYNAMIC_DRAW);
	mom_compute_ssbo_read.allocate(moms, GL_DYNAMIC_DRAW);
	mom_compute_ssbo_write.allocate(moms, GL_DYNAMIC_DRAW);

	LoadShaders();

	// Set up the VBO
	ff_vbo.setVertexBuffer(ff_vertex_buffer, 4, 0);
	ff_vbo.setColorBuffer(ff_compute_ssbo_read, sizeof(ComputeFirefly), sizeof(float) * 2 + sizeof(int) * 2);
	ff_vbo.setAttributeBuffer(
		ff_geo_shader.getAttributeLocation("velocity"),
		ff_compute_ssbo_read,
		3,
		sizeof(ComputeFirefly),
		sizeof(float) * 2 + sizeof(int) * 2 + sizeof(glm::vec4)
	);
	ff_vbo.enableColors();

	mom_vbo.setVertexBuffer(mom_vertex_buffer, 4, 0);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ff_positions_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ff_directions_ssbo);
	ff_vertex_buffer.bindBase(GL_SHADER_STORAGE_BUFFER, 2);
	ff_compute_ssbo_read.bindBase(GL_SHADER_STORAGE_BUFFER, 3);
	ff_compute_ssbo_write.bindBase(GL_SHADER_STORAGE_BUFFER, 4);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mom_positions_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, mom_directions_ssbo);
	mom_vertex_buffer.bindBase(GL_SHADER_STORAGE_BUFFER, 7);
	mom_compute_ssbo_read.bindBase(GL_SHADER_STORAGE_BUFFER, 8);
	mom_compute_ssbo_write.bindBase(GL_SHADER_STORAGE_BUFFER, 9);

	assert(ff_positions_ssbo != 0 && ff_directions_ssbo != 0 && ff_vertex_buffer.getId() != 0 
		&& ff_compute_ssbo_read.getId() != 0 && ff_compute_ssbo_write.getId() != 0
		&& mom_positions_ssbo != 0 && mom_directions_ssbo != 0 
		&& mom_compute_ssbo_read.getId() != 0 && mom_compute_ssbo_write.getId() != 0);

	camera.setFov(120.f);
	camera.setFarClip(512.f);
	camera.setAspectRatio(16.f / 9.f);

	box.setPosition(glm::vec3(0.f));
	box.setScale(glm::vec3(0.1f));

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
	glDeleteBuffers(1, &mom_positions_ssbo);
	glDeleteBuffers(1, &mom_directions_ssbo);
}

PinInput* Fireflies::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* Fireflies::PinOutputs(size_t& size) {
	size = 1;
	return &pinOutFbo;
}

void Fireflies::Update(UpdateParams* params) {
	mom_compute_shader.begin();
	mom_compute_shader.setUniform1f("time", params->time);
	mom_compute_shader.setUniform1f("maxVelocity", maxVelocity * 1.1f);
	mom_compute_shader.dispatchCompute(1, 1, 1);
	mom_compute_shader.end();

	mom_compute_ssbo_write.copyTo(mom_compute_ssbo_read);

	ofShader* compute_shader = useAxisAlignedCompute > 0.f
		? &ff_compute_axis_aligned_shader : &ff_compute_shader;
	
	compute_shader->begin();
	compute_shader->setUniform1f("time", params->time * 0.05);
	compute_shader->setUniform1f("maxVelocity", maxVelocity);
	compute_shader->dispatchCompute((fireflies_count + 1024 - 1) / 1024, 1, 1);
	compute_shader->end();

	// Ping-pong copy written compute results back to the compute read buffer.
	ff_compute_ssbo_write.copyTo(ff_compute_ssbo_read);
	// Also ping-pong copy positions back, these are stored separately since positions are mapped for CPU reading.
	glCopyNamedBufferSubData(ff_vertex_buffer.getId(), ff_positions_ssbo, 0, 0, sizeof(glm::vec4) * fireflies_count);
	glCopyNamedBufferSubData(mom_vertex_buffer.getId(), mom_positions_ssbo, 0, 0, sizeof(glm::vec4) * moms_count);

	if (invalidate_posmap) {
		invalidate_posmap = false;
		glMemoryBarrier(GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
	}

	if (invalidate_dirmap) {
		invalidate_dirmap = false;
		glFlushMappedNamedBufferRange(ff_directions_ssbo, 0, sizeof(glm::vec4) * fireflies_count);
		glFlushMappedNamedBufferRange(mom_directions_ssbo, 0, sizeof(glm::vec4) * moms_count);
	}

	pinOutFbo.DirtyConnections();
}

void Fireflies::Draw(DrawParams* params) {
	fbo.begin();
	fbo.clearColorBuffer(0.f);
	ofEnableBlendMode(OF_BLENDMODE_ADD);

	camera.begin();
	ff_geo_shader.begin();

	float pulse_distance = camera.getNearClip() 
		+ pulseCameraDistance * (camera.getFarClip() - camera.getNearClip());

	ff_geo_shader.setUniform3f("cameraLeft", camera.getSideDir());
	ff_geo_shader.setUniform3f("cameraUp", camera.getUpAxis());
	ff_geo_shader.setUniform1f("pulseDistance", pulse_distance);
	ff_geo_shader.setUniform1f("pulseRange", pulseRange);

	ff_vbo.draw(GL_POINTS, 0, fireflies_count);

	ff_geo_shader.end();

	// glPointSize(8);
	// mom_vbo.draw(GL_POINTS, 0, moms_count);

	// box.draw();

	camera.end();
	fbo.end();
}

bool Fireflies::LoadShaders() {
	const std::filesystem::path shaders_path = std::filesystem::current_path() / "data" / "shaders";
	const std::string compute_name("fireflies-compute.glsl");
	const std::string compute_aa_name("fireflies-compute-axis-aligned.glsl");
	const std::string vert_frag_name("fireflies");

	bool compute_ff_loaded = ff_compute_shader.setupShaderFromFile(GL_COMPUTE_SHADER, shaders_path / compute_name)
		&& ff_compute_shader.linkProgram();

	bool compute_ffaa_loaded = ff_compute_axis_aligned_shader.setupShaderFromFile(GL_COMPUTE_SHADER, shaders_path / compute_aa_name)
		&& ff_compute_axis_aligned_shader.linkProgram();

	bool compute_moms_loaded = mom_compute_shader.setupShaderFromFile(GL_COMPUTE_SHADER, shaders_path / "fireflies-moms-compute.glsl")
		&& mom_compute_shader.linkProgram();

	ff_geo_shader.setGeometryInputType(GL_POINTS);
	ff_geo_shader.setGeometryOutputType(GL_TRIANGLE_STRIP);
	ff_geo_shader.setGeometryOutputCount(4);

	bool geo_loaded = ff_geo_shader.load(
		shaders_path / (vert_frag_name + ".vert"),
		shaders_path / (vert_frag_name + ".frag"),
		shaders_path / (vert_frag_name + "-geo.glsl")
	);

	bool test_loaded = test_shader.load(shaders_path / (vert_frag_name + ".vert"), shaders_path / (vert_frag_name + ".frag"));
	assert(test_loaded);

	if (!compute_ff_loaded) {
		printf("Failed to load or link fireflies compute shader\n");
	}

	if (!compute_ffaa_loaded) {
		printf("Failed to load or link fireflies axis-aligned compute shader\n");
	}

	if (!compute_moms_loaded) {
		printf("Failed to load or link fireflies moms compute shader\n");
	}

	if (!geo_loaded) {
		printf("Failed to load or link fireflies geo/vert/frag shaders\n");
	}
	
	return compute_ff_loaded && compute_ffaa_loaded && geo_loaded && compute_moms_loaded;
}

bool Fireflies::GuiDrawPropertiesList(UpdateParams* params) {
	if (ImGui::Button("refresh")) {
		LoadShaders();
		return true;
	}
	return false;
}

void Fireflies::AvoidanceLoop() {
	assert(ff_posmap != nullptr && ff_dirmap != nullptr && mom_posmap != nullptr && mom_dirmap != nullptr);
	std::vector<glm::vec4*> found;

	while (!stop) {
		// Request a refresh of mapped positions.
		invalidate_posmap = true;

		auto start = std::chrono::high_resolution_clock::now();

		UpdateDirections(ff_octree, ff_positions, ff_dirmap, fireflies_count, found, avoidanceRadius);
		UpdateDirections(mom_octree, mom_positions, mom_dirmap, moms_count, found, momAvoidanceRadius);

		auto stop = std::chrono::high_resolution_clock::now();
		// printf("took %llu ms to update directions\n", (stop - start).count() / 100000);
		
		// Ask the main thread to flush the segment of the direction buffer we just wrote to.
		invalidate_dirmap = true;

		start = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < moms_count; i++) {
			glm::vec3 old_pos = mom_positions[i];
			glm::vec3 new_pos = mom_posmap[i];
			mom_octree.Update(&mom_positions[i], old_pos, new_pos);
			// Make sure to update the mom's position now so any re-organization the octree does can reference the new position.
			mom_positions[i] = glm::vec4(new_pos, 1.f);
		}

		// Update the octree's positions.
		for (size_t i = 0; i < fireflies_count; i++) {
			glm::vec3 old_pos = ff_positions[i];
			glm::vec3 new_pos = ff_posmap[i];
			ff_octree.Update(&ff_positions[i], old_pos, new_pos);
			// Make sure to update the particle's position now so any re-organization the octree does can reference the new position.
			ff_positions[i] = glm::vec4(new_pos, 1.f);
		}

		stop = std::chrono::high_resolution_clock::now();
		/// printf("took %llu ms to update positions\n", (stop - start).count() / 100000);
	}
}

void Fireflies::UpdateDirections(
	Octree<glm::vec4, 8>& octree,
	const std::vector<glm::vec4>& positions, 
	glm::vec4* dirmap, 
	size_t count,
	std::vector<glm::vec4*>& found, 
	float avoidance_radius
) {
	float max_force = 0.f;
	for (size_t i = 0; i < count; i++) {
		glm::vec3 dir(0.f);
		glm::vec3 my_pos = positions[i];
		// For each particle, look nearby.
		octree.FindItems(my_pos, avoidance_radius, found);
		// Each nearby particle exerts a force based on how far away it is.
		// Closer particles exert more force.-
		for (size_t j = 0; j < found.size(); j++) {
			// Make sure this particle gets skipped (it will show up in the result).
			if (&positions[i] == found[j]) {
				continue;
			}

			glm::vec3 their_pos = *found[j];
			glm::vec3 away_from_them = glm::normalize(my_pos - their_pos);
			float distance = glm::distance(my_pos, their_pos);
			// When almost at the edge, the force will be very low.
			float rev_dist01 = (avoidance_radius - distance) / avoidance_radius;
			float force = pow(rev_dist01, 4);
			max_force = std::max(force, max_force);
			dir = dir + away_from_them * force;
		}

		// If more items than just this one was found...
		if (found.size() > 1) {
			dir = glm::normalize(dir) * max_force;
		}

		dirmap[i] = glm::vec4(dir, 1.f);
	}
}
