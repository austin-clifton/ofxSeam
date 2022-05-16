#pragma once

#include "i-node.h"
#include "../pins/pin-note-event.h"

using namespace seam::pins;

namespace seam::nodes {
	// TODO: this probably doesn't belong with the "regular" seam nodes.
	// This is an example of notes-to-shader, not meant to be re-used in any way.
	class ForceGrid : public INode {
	public:
		ForceGrid();
		virtual ~ForceGrid();

		void Draw(DrawParams* params) override;

		void Update(UpdateParams* params) override;

		bool GuiDrawPropertiesList() override;

		IPinInput** PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

	private:
		// contains the data needed for CPU side animation into the NoteForceGpu struct.
		struct NoteForceAnim {
			// Forces animate in, are alive for awhile, and then animate out
			enum ForceState : uint8_t {
				BIRTH,
				LIVE,
				DYING
			};

			ForceState state = ForceState::BIRTH;
			// helps us keep a pair of on/off events together
			int instance_id = 0;
			// frequency in HERTZ of the input note
			float freq_hz = 0.0f;
			// the incoming velocity of a MIDI note determines max NoteForceGpu.rForce
			float note_vel = 0.0f;
			// radius target is based on minimum/maximum live frequencies
			float r_target = 0.f;
			// radial velocity is affected by acceleration as a factor of current radial offset vs target offset
			float r_vel = 0.0f;
			// animation time increases during birth and death
			float time = 0.f;
		};

		// contains the data needed to animate each "force point" in the shader.
		struct NoteForceGpu {
			// radial offset 0..1 (0 is center, 1 is edge)
			float r_offset = 0.f;
			// angle around the center (polar coordinates)
			float theta = 0.f;
			// Noise offset factor applied to rOffset. Increase for noised force motion.
			float n_offset = 0.f;
			// (-inf, inf) noise "height":
			// internally noise is applied on a cylinder, so varying heights will change the noise's motion.
			float n_height = 0.f;
			
			// (-inf, inf) but once you make this too large shapes will stop rendering correctly,
			// since the sample area for each pixel is an NxN grid of shapes around the shape it's closest to.
			float r_force = 0.0f;
			// minimum force, affects shapes past the edge of the circle dictated by rForce.
			// set to 0 for a perfect grid outside of the force area, but a tiny minimum force is more fun!
			float min_force = 0.03f;

			// shapes' sizes within the force area are increased or decreased by this factor
			float size_factor = 1.f;
		};

		struct NoteForce {
			NoteForceGpu gpu;
			NoteForceAnim cpu;
		};

		inline void AnimateBirth(NoteForce& force);
		inline void AnimateLife(NoteForce& force);
		inline void AnimateDying(NoteForce& force);

		// TODO these should probs be editable fields
		float birth_time = 0.15f;
		float death_time = 0.5f;
		float max_force = 0.2f;
		float max_radius = 0.5f;
		float max_acceleration = 0.05f;
		float max_rotation_factor = 0.1f;

		// cached for calculations
		int freq_edge_inner = 0;
		int freq_edge_outer = 0;

		std::array<NoteForce, 8> note_forces;
		uint8_t note_forces_size = 0;

		glm::ivec2 tex_size = glm::ivec2(1920, 1080);
		const std::string SHADER_NAME = "force-grid";
		ofShader shader;
		ofFbo fbo;

		PinFloat<1> pin_time = PinFloat<1>("time");

		PinNoteEvent<0> pin_notes_on_stream = PinNoteEvent<0>(
			"notes on stream",
			"incoming stream of note ON events",
			notes::EventTypes::ON
		);

		PinNoteEvent<0> pin_notes_off_stream = PinNoteEvent<0>(
			"notes off stream",
			"incoming stream of note OFF events",
			notes::EventTypes::OFF
		);

		PinNoteEvent<1> pin_kick_drum_on = PinNoteEvent<1>(
			"kick drum on",
			"hook the MIDI note for the kick drum to this pin",
			notes::EventTypes::ON
		);

		std::array<IPinInput*, 4> pin_inputs = {
			&pin_time,
			&pin_notes_on_stream,
			&pin_notes_off_stream,
			&pin_kick_drum_on
		};

		PinOutput pin_out_material = pins::SetupOutputPin(this, pins::PinType::MATERIAL, "material");
	};
}