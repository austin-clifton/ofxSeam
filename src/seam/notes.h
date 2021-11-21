namespace seam {
	struct BasicNote {
		// velocity
		float vel;
		// pitch in cycles per second
		// 440 is A, and MIDI key 69
		float pitch;
	};

	// supercollider notes are analogous to individual synth instances
	struct SCNote : public BasicNote {
		// TODO synth id, what else...?
		// SC synths might have a variable number of input params,
		// and new values can be streamed into them at runtime
	};

	// TODO
	// osc
	// vst?
	// etc.
}
