#pragma once

#include "i-node.h"

// #include "Gist.h"

using namespace seam::pins;

namespace seam::nodes {

	class GistAudio : public INode, public IAudioNode {
	public:
		GistAudio();
		~GistAudio();

		void Update(UpdateParams* params) override;

		void ProcessAudio(ProcessAudioParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		void GuiDrawNodeCenter() override;
	
	private:
		struct SampleAcc {
			float acc;
			int samples;
			float avg;
		};

		void UpdateSampleAcc(SampleAcc& sacc, float value);
		void ClearSampler(SampleAcc& sacc);
		void DrawSampler(SampleAcc sacc, const char* label);

		// std::unique_ptr<Gist<float>> gist;

		SampleAcc rms;
		SampleAcc peak;
		SampleAcc energyDiff;
		SampleAcc hfc;
		SampleAcc spectralCentroid;
		SampleAcc spectralFlat;
	};
}