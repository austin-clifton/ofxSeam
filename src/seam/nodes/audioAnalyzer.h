#pragma once

#include "ofxAudioAnalyzer.h"

#include "seam/nodes/i-node.h"

using namespace seam::pins;

namespace seam::nodes {
	class AudioAnalyzer : public INode, public IAudioNode {
	public:
		AudioAnalyzer(const ofSoundStreamSettings& soundSettings);
		~AudioAnalyzer();

		void Update(UpdateParams* params) override;

		PinInput* PinInputs(size_t& size) override;

		PinOutput* PinOutputs(size_t& size) override;

		void GuiDrawNodeCenter() override;

		bool GuiDrawPropertiesList() override;

		void ProcessAudio(ofSoundBuffer& input) override;
	private:
		struct AudioAlgorithm {
			AudioAlgorithm(ofxAAAlgorithm _alg, const char* _name, ImVec2 _limits = ImVec2(0,1)) {
				algorithm = _alg;
				name = _name;
				limits = _limits;
			}

			ofxAAAlgorithm algorithm;
			const char* name;
			ImVec2 limits;

			bool enabled = false;
		};

		struct AudioAlgorithmMulti : public AudioAlgorithm {
			AudioAlgorithmMulti(ofxAAAlgorithm _alg, const char* _name, ImVec2 _limits = ImVec2(0,1)) 
				: AudioAlgorithm(_alg, _name, _limits) {
			}

			std::vector<float> values0;
			std::vector<float> values1;
		};

		struct AudioAlgorithmLinePlot : public AudioAlgorithm {
			AudioAlgorithmLinePlot(ofxAAAlgorithm _alg, const char* _name, ImVec2 _limits = ImVec2(0,1)) 
				: AudioAlgorithm(_alg, _name, _limits) {
				assert(values0.size() == values1.size());
			}

			const static size_t MAX_SAMPLES = 120;

			void AddPoint(float f0, float f1) {
				values0[offset] = f0;
				values1[offset] = f1;
				offset = (offset + 1) % values0.size();
			}

			std::array<float, MAX_SAMPLES> values0 = { 0.f };
			std::array<float, MAX_SAMPLES> values1 = { 0.f };
			size_t offset = 0;
		};

		std::array<AudioAlgorithmMulti, 2> multiValueAlgos = {
			AudioAlgorithmMulti(ofxAAAlgorithm::SPECTRUM, "Spectrum", ImVec2(-6, 0)),
			//AudioAlgorithmMulti(ofxAAAlgorithm::MEL_BANDS, "Mel Bands"),
			//AudioAlgorithmMulti(ofxAAAlgorithm::MFCC, "MFCC"),
			AudioAlgorithmMulti(ofxAAAlgorithm::HPCP, "HPCP", ImVec2(0, 1)),
			//AudioAlgorithmMulti(ofxAAAlgorithm::MULTI_PITCHES, "Multi Pitches"),
			//AudioAlgorithmMulti(ofxAAAlgorithm::TRISTIMULUS, "Tristimulus"),
		};

		std::array<AudioAlgorithmLinePlot, 2> linePlotAlgos = {
			AudioAlgorithmLinePlot(ofxAAAlgorithm::RMS, "RMS"),
			AudioAlgorithmLinePlot(ofxAAAlgorithm::INHARMONICITY, "Inharmonicity")
		};
		
		ofxAudioAnalyzer audioAnalyzer;
		float audioSample = -1.f;

		float alpha = 0.1f;
		float silenceThresh = 0.02f;
		float timeThresh = 100.f;
		bool useTimeThresh = true;

		bool enableOnsets = false;
		bool onsetOccurred = false;
	};
}