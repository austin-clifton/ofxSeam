#include <cstdio>
#include <filesystem>
#include "forceGrid.h"
#include "seam/imguiUtils/properties.h"
#include "seam/shaderUtils.h"

using namespace seam::pins;
using namespace seam::nodes;
using namespace seam::notes;

ForceGrid::ForceGrid() : INode("Force Grid") {
	flags = (NodeFlags)(flags | NodeFlags::IS_VISUAL);
	pinNotesOnStream = &pin_inputs[2];
	pinNotesOffStream = &pin_inputs[3];

	fbo.allocate(tex_size.x, tex_size.y);

	if (ShaderUtils::LoadShader(shader, SHADER_NAME)) {
		shader.setUniform2iv("resolution", &tex_size[0]);
	} else {
		printf("force grid shader at path %s failed to load\n", SHADER_NAME.c_str());
	}

	gui_display_fbo = &fbo;
}

ForceGrid::~ForceGrid() {
	// NOOP?
}

PinInput* ForceGrid::PinInputs(size_t& size) {
	size = pin_inputs.size();
	return pin_inputs.data();
}

PinOutput* ForceGrid::PinOutputs(size_t& size) {
	size = 1;
	return &pin_out_material;
}

bool ForceGrid::GuiDrawPropertiesList(UpdateParams* params) {
	// TODO? there may be nothing to do here
	return false;
}

void ForceGrid::Draw(DrawParams* params) {
	fbo.begin();
	shader.begin();

	// TODO set uniforms

	fbo.draw(0, 0);
	
	shader.end();
	fbo.end();
}

inline void ForceGrid::AnimateBirth(NoteForce& force) {
	// calculate normalized "birth time"
	const float nt = force.cpu.time / birth_time;
	
	// force increases from 0 to max during birth period
	force.gpu.r_force = nt * force.cpu.note_vel * max_force;
	// min force also increases during birth period
	force.gpu.min_force = force.gpu.r_force * 0.1f;
}

inline void ForceGrid::AnimateLife(NoteForce& force) {
	// TODO? life is the "stable state"
}

inline void ForceGrid::AnimateDying(NoteForce& force) {
	// calculate normalized "death time"
	const float nt = force.cpu.time / death_time;

	// force decreases from max to 0 during death period
	force.gpu.r_force = (1.0f - nt) * force.cpu.note_vel * max_force;
	// min force dies off during death too
	force.gpu.min_force = force.gpu.r_force * 0.1f;
}

void ForceGrid::Update(UpdateParams* params) {
	using ForceState = NoteForceAnim::ForceState;

	bool notes_list_changed = false;

	// advance time on existing particles,
	// push them along their animation curves,
	// and kill any which are dead
	for (int i = 0; i < note_forces_size; i++) {
		NoteForce& nf = note_forces[i];
		nf.cpu.time += params->delta_time;

		switch (nf.cpu.state) {
		case ForceState::BIRTH:
			if (nf.cpu.time > birth_time) {
				// transition to LIVE state
				nf.cpu.state = ForceState::LIVE;
				nf.cpu.time = 0.f;
				AnimateLife(nf);
			}
			AnimateBirth(nf);
			break;

		case ForceState::LIVE:
			// notes don't transition to dying until a MIDI note off event is received
			// TODO a fail-safe death after some time may be desirable here
			AnimateLife(nf);
			break;

		case ForceState::DYING:
			if (nf.cpu.time > death_time) {
				// DIE
				// replace with the force in the back and decrement note_forces_size
				note_forces[i] = note_forces[note_forces_size - 1];
				note_forces_size--;
				notes_list_changed = true;
				// decrement i since we want to reprocess the current element now
				i--;
			} else {
				AnimateDying(nf);
			}
			break;
		}
	}

	// drain and handle note events

	// note ON events create new particles as long as there's space for them
	size_t size;
	NoteOnEvent** onEvents = (NoteOnEvent**)pinNotesOnStream->GetEvents(size);
	for (size_t i = 0; i < size && note_forces_size < note_forces.size(); i++) {
		NoteOnEvent* on_ev = onEvents[i];
		// set the two pieces of info from the on event: frequency and note velocity
		NoteForce& force = note_forces[note_forces_size];
		force.cpu.freq_hz = on_ev->frequency;
		force.cpu.note_vel = on_ev->velocity;
		// also seed theta with a random value (from here out it will be ever-increasing)
		force.gpu.theta = ofRandomuf() * TWO_PI;

		note_forces_size++;
		notes_list_changed = true;
	}
	pinNotesOnStream->ClearEvents(size);

	// note OFF events transition live notes into the DYING state
	NoteOffEvent** offEvents = (NoteOffEvent**)pinNotesOffStream->GetEvents(size);

	// TODO something with note off events...????

	pinNotesOffStream->ClearEvents(size);

	/*
	while (pin_notes_off_stream.channels.size()) {
		NoteOffEvent* off_ev = (NoteOffEvent*)pin_notes_off_stream[pin_notes_off_stream.channels.size() - 1];

		// find the corresponding force by instance id
		const auto it = std::find(
			note_forces.begin(), 
			note_forces.begin() + note_forces_size, 
			[off_ev](NoteForce nf) { return nf.cpu.instance_id == off_ev->instance_id; }
		);
		assert(it != note_forces.end());
		it->cpu.state = ForceState::DYING;
		it->cpu.time = 0.f;

		pin_notes_off_stream.channels.pop_back();
	}
	*/

	// recalculate the target radius for each note, if the notes list changed
	if (notes_list_changed) {
		// first loop, find min/max frequencies using MIDI "frequency" stored in the instance_id...
		// MIDI notes are already scaled nicely for this purpose
		int min_freq = std::numeric_limits<int>::max();
		int max_freq = std::numeric_limits<int>::min();
		for (uint8_t i = 0; i < note_forces_size; i++) {
			min_freq = std::min(note_forces[i].cpu.instance_id, min_freq);
			max_freq = std::max(note_forces[i].cpu.instance_id, max_freq);
		}

		freq_edge_inner = min_freq - 3;
		freq_edge_outer = max_freq + 3;
		const int freq_range = freq_edge_outer - freq_edge_inner;

		// now calculate target radii
		for (uint8_t i = 0; i < note_forces_size; i++) {
			NoteForce& nf = note_forces[i];

			// calculate a target radius relative to the min/max frequencies;
			// lower frequencies shift towards the outer edge of the circle
			nf.cpu.r_target = max_radius
				- ( (nf.cpu.instance_id - freq_edge_inner) / freq_range * max_radius );
		}
	}

	// animate forces accelerating towards their target radii,
	// and animate rotation of forces around their circles
	for (uint8_t i = 0; i < note_forces_size; i++) {
		NoteForce& nf = note_forces[i];
		
		// handle radial shift;
		// accelerate faster when the target is further away
		float a = nf.cpu.r_target - nf.gpu.r_offset;
		// clamp it to prevent craziness
		a = std::min(max_acceleration, std::max(-max_acceleration, a));
		// apply acceleration to velocity, and velocity to the radial offset
		nf.cpu.r_vel += a;
		nf.gpu.r_offset += nf.cpu.r_vel;

		// handle rotation
		// higher notes (closer to the circle's center) move faster
		float rot_factor = (max_radius - nf.cpu.r_target) * max_rotation_factor;
		nf.gpu.theta += rot_factor * params->delta_time;
	}

}