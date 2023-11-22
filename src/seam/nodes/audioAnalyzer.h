#pragma once

#if BUILD_AUDIO_ANALYSIS

#include "implot.h"
#include "ofxAudioAnalyzer.h"

#include "seam/nodes/iNode.h"

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

		bool GuiDrawPropertiesList(UpdateParams* params) override;

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
			AudioAlgorithmMulti(
				ofxAAAlgorithm _alg, 
				const char* _name, 
				PinOutput* _pinOutChannels = nullptr, 
				PinOutput* _pinOutChannelsSize = nullptr, 
				ImVec2 _limits = ImVec2(0,1), 
				ImPlotScale _scale = ImPlotScale_Linear
			) : AudioAlgorithm(_alg, _name, _limits) 
			{
				pinOutChannels = _pinOutChannels;
				pinOutChannelsSize = _pinOutChannelsSize;
				scale = _scale;
			}

			std::vector<float> values0;
			std::vector<float> values1;

			PinOutput* pinOutChannels = nullptr;
			PinOutput* pinOutChannelsSize = nullptr;

			ImPlotScale scale;
		};

		struct AudioAlgorithmLinePlot : public AudioAlgorithm {
			AudioAlgorithmLinePlot(ofxAAAlgorithm _alg, const char* _name,
				PinOutput* _pinOutValue = nullptr, ImVec2 _limits = ImVec2(0,1)
			) : AudioAlgorithm(_alg, _name, _limits) 
			{
				assert(values0.size() == values1.size());
				pinOutValue = _pinOutValue;
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

			float lastValue = -1.f;
			PinOutput* pinOutValue = nullptr;
		};

		std::array<PinOutput, 6> pinOutputs = {
			SetupOutputPin(this, PinType::FLOAT, "RMS"),
			SetupOutputPin(this, PinType::FLOAT, "Spectrum Channels"),
			SetupOutputPin(this, PinType::FLOAT, "Spectrum Size"),
			SetupOutputPin(this, PinType::FLOAT, "HPCP"),
			SetupOutputPin(this, PinType::FLOAT, "Mel Bands"),
			SetupOutputPin(this, PinType::FLOAT, "Mel Bands Size"),
		};

		std::array<AudioAlgorithmMulti, 3> multiValueAlgos = {
			AudioAlgorithmMulti(
				ofxAAAlgorithm::MEL_BANDS, 
				"Mel Bands", 
				FindPinOutByName(this, "Mel Bands"), 
				FindPinOutByName(this, "Mel Bands Size"), 
				ImVec2(DB_MIN, DB_MAX)
			),

			/*
			AudioAlgorithmMulti(
				ofxAAAlgorithm::MEL_BANDS,
				"Mel Bands",
				FindPinOutByName(this, "Mel Bands"),
				FindPinOutByName(this, "Mel Bands Size"),
				ImVec2(DB_MIN, DB_MAX),
			),
			*/

			AudioAlgorithmMulti(
				ofxAAAlgorithm::SPECTRUM, 
				"Spectrum",
				FindPinOutByName(this, "Spectrum Channels"), 
				FindPinOutByName(this, "Spectrum Size"),
				ImVec2(DB_MIN, DB_MAX),
				ImPlotScale_Log10
			), 

			AudioAlgorithmMulti(
				ofxAAAlgorithm::HPCP, 
				"HPCP", 
				FindPinOutByName(this, "HPCP"), 
				nullptr, 
				ImVec2(0, 1)
			),

			//AudioAlgorithmMulti(ofxAAAlgorithm::MFCC, "MFCC"),
			//AudioAlgorithmMulti(ofxAAAlgorithm::MULTI_PITCHES, "Multi Pitches"),
			//AudioAlgorithmMulti(ofxAAAlgorithm::TRISTIMULUS, "Tristimulus"),
		};

		std::array<AudioAlgorithmLinePlot, 2> linePlotAlgos =  {
			AudioAlgorithmLinePlot(ofxAAAlgorithm::RMS, "RMS", FindPinOutByName(this, "RMS")),
			AudioAlgorithmLinePlot(ofxAAAlgorithm::INHARMONICITY, "Inharmonicity")
		};
		
		ofxAudioAnalyzer audioAnalyzer;
		float lastRms = -1.f;

		float alpha = 0.1f;
		float silenceThresh = 0.02f;
		float timeThresh = 100.f;
		bool useTimeThresh = true;

		bool enableOnsets = false;
		bool onsetOccurred = false;
	};
}

#endif // BUILD_AUDIO_ANALYSIS