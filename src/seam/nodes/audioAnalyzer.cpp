#if BUILD_AUDIO_ANALYSIS

#include "audioAnalyzer.h"

using namespace seam;
using namespace seam::nodes;

AudioAnalyzer::AudioAnalyzer(const ofSoundStreamSettings& settings) : INode("Audio Analyzer") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_OVER_TIME);

    audioAnalyzer.setup(settings.sampleRate, settings.bufferSize, settings.numInputChannels);
	// Disable all the audio analyzer algorithms by default, enable _some_ back when audio is actually playing.
	for (size_t i = 0; i < settings.numInputChannels; i++) {
		for (size_t j = 0; j <= ofxAAAlgorithm::ONSETS; j++) {
			audioAnalyzer.setActive(i, (ofxAAAlgorithm)j, false);
		}
	}

	// ...Except RMS, we want RMS enabled at all times so we know when to re-enable other stuff.
	for (size_t i = 0; i < settings.numInputChannels; i++) {
		audioAnalyzer.setActive(i, ofxAAAlgorithm::RMS, true);
	}

	audioAnalyzer.setOnsetsParameters(0, alpha, silenceThresh, timeThresh, useTimeThresh);
}

AudioAnalyzer::~AudioAnalyzer() {
	audioAnalyzer.exit();
}

PinInput* AudioAnalyzer::PinInputs(size_t& size) {
	size = 0;
    return nullptr;
}

PinOutput* AudioAnalyzer::PinOutputs(size_t& size) {
	size = pinOutputs.size();
	return pinOutputs.data();
}

void AudioAnalyzer::Update(UpdateParams* params) {
	// Always push RMS.
	params->push_patterns->Push(pinOutputs[0], &lastRms, 1);

	for (size_t i = 0; i < multiValueAlgos.size(); i++) {
		auto& algo = multiValueAlgos[i];
		if (multiValueAlgos[i].enabled) {
			if (algo.pinOutChannelsSize != nullptr) {
				uint32_t channelsSize = (uint32_t)algo.values0.size();
				params->push_patterns->Push(*algo.pinOutChannelsSize, &channelsSize, 1);
			}
			params->push_patterns->Push(*algo.pinOutChannels, algo.values0.data(), algo.values0.size());
			std::fill(algo.values0.begin(), algo.values0.end(), 0.0f);
			std::fill(algo.values1.begin(), algo.values1.end(), 0.0f);
		}
	}

	// Reset lastRms now that it's been sent along.
	lastRms = 0.f;
}

void AudioAnalyzer::ProcessAudio(ofSoundBuffer& input) {
	audioAnalyzer.analyze(input);
	float currRms = audioAnalyzer.getValue(ofxAAAlgorithm::RMS, 0);
	lastRms = std::max(lastRms, currRms);

	for (size_t algIndex = 0; algIndex < multiValueAlgos.size(); algIndex++) {
		auto& algo = multiValueAlgos[algIndex];
		auto& vals0 = audioAnalyzer.getValues(algo.algorithm, 0);
		// auto& vals1 = audioAnalyzer.getValues(algo.algorithm, 1);

		for (size_t i = 0; i < vals0.size() && i < algo.values0.size(); i++) {
			algo.values0[i] = std::max(vals0[i], algo.values0[i]);
		}
	} 

	onsetOccurred = onsetOccurred || (enableOnsets && audioAnalyzer.getOnsetValue(0));
}

void AudioAnalyzer::GuiDrawNodeCenter() {

}

bool AudioAnalyzer::GuiDrawPropertiesList(UpdateParams* params) {
	// TODO:
	// Display a checkmark or a lit up thingy if we're detecting audio input (RMS > 0)
	if (ImGui::Checkbox("Beat Tracking", &enableOnsets)) {
		for (int i = 0; i < audioAnalyzer.getChannelsNum(); i++) {
			audioAnalyzer.setActive(i, ofxAAAlgorithm::ONSETS, enableOnsets);
		}
	}

	if (enableOnsets) {
		bool onsetParamsChanged = ImGui::DragFloat("Alpha", &alpha);
		onsetParamsChanged = ImGui::DragFloat("Silence Threshold", &silenceThresh) || onsetParamsChanged;
		onsetParamsChanged = ImGui::DragFloat("Time Threshold", &timeThresh) || onsetParamsChanged;
		onsetParamsChanged = ImGui::Checkbox("Use Time Thresh", &useTimeThresh) || onsetParamsChanged;

		if (onsetParamsChanged) {
			audioAnalyzer.setOnsetsParameters(0, alpha, silenceThresh, timeThresh, useTimeThresh);
		}

		if (onsetOccurred) {
			ImGui::SameLine();
			ImGui::Text("BEAT");
			onsetOccurred = false;
		}
	}

	// Display check boxes for display of each available algorithm.
	bool multiChanged = false;
	ImGui::Text("Multi Value Algorithms:");
	for (size_t i = 0; i < multiValueAlgos.size(); i++) {
		multiChanged = multiChanged || ImGui::Checkbox(multiValueAlgos[i].name, &multiValueAlgos[i].enabled);
		ImGui::SameLine();
	}

	ImGui::NewLine();
	ImGui::Text("Single Value Algorithms (line plot over time):");
	bool singleChanged = false;
	for (size_t i = 0; i < linePlotAlgos.size(); i++) {
		singleChanged = singleChanged || ImGui::Checkbox(linePlotAlgos[i].name, &linePlotAlgos[i].enabled);
		ImGui::SameLine();
	}

	if (multiChanged) {
		printf("multi value algorithms enabled changed\n");
		for (size_t i = 0; i < multiValueAlgos.size(); i++) {
			auto& algo = multiValueAlgos[i];
			for (int j = 0; j < audioAnalyzer.getChannelsNum(); j++) {
				audioAnalyzer.setActive(j, algo.algorithm, algo.enabled);
				// Make sure the vector storage for this algo is allocated.
				auto& values = audioAnalyzer.getValues(algo.algorithm, 0);
				algo.values0.resize(values.size(), 0.f);
				algo.values1.resize(values.size(), 0.f);
			}
		}
	}

	if (singleChanged) {
		printf("single value algorithms enabled changed\n");
		for (size_t i = 0; i < linePlotAlgos.size(); i++) {
			auto& algo = linePlotAlgos[i];
			for (int j = 0; j < audioAnalyzer.getChannelsNum(); j++) {
				audioAnalyzer.setActive(j, algo.algorithm, algo.enabled);
			}
		}
	}
	
	ImGui::NewLine();
	
	const double xScale = 44100.0 / 256.0;

	// Plot multi value algos with histograms
	for (size_t i = 0; i < multiValueAlgos.size(); i++) {
		if (i % 2 == 1) {
			ImGui::SameLine();
		}

		auto& algo = multiValueAlgos[i];
		if (algo.enabled && ImPlot::BeginPlot(algo.name, ImVec2(512,512))) {
			ImPlot::SetupAxisLimits(ImAxis_Y1, algo.limits.x, algo.limits.y, ImPlotCond_Always);
			ImPlot::SetupAxisScale(ImAxis_X1, algo.scale);

			ImPlot::PlotLine("left", algo.values0.data(), algo.values0.size(), xScale);
			ImPlot::PlotLine("right", algo.values1.data(), algo.values1.size(), xScale);
			
			/*
			ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
			ImPlot::PlotHistogram(algo.name, values.data(), values.size(), 
				ImPlotBin_Sturges, 1.0, ImPlotRange(), ImPlotHistogramFlags_Density);
			*/

			ImPlot::EndPlot();
		}
	}

	// Plot single value algos using line plots
	for (size_t i = 0; i < linePlotAlgos.size(); i++) {
		if (i % 2 == 1) {
			ImGui::SameLine();
		}

		auto& algo = linePlotAlgos[i];
		if (algo.enabled && ImPlot::BeginPlot(algo.name, ImVec2(200, 200))) {
			// TODO use audio sample from audio loop RMS...
			float f0 = audioAnalyzer.getValue(algo.algorithm, 0);
			float f1 = audioAnalyzer.getValue(algo.algorithm, 1);
			algo.AddPoint(f0, f1);
			ImPlot::SetupAxisLimits(ImAxis_Y1, algo.limits.x, algo.limits.y);
			// ImPlot::SetupAxisScale(ImAxis_Y1, ImPlotScale_SymLog);
			ImPlot::PlotLine("left", algo.values0.data(), algo.values0.size(), 1.0, 0, algo.offset);
			ImPlot::PlotLine("right", algo.values1.data(), algo.values1.size(), 1.0, 0, algo.offset);

			ImPlot::EndPlot();
		}
	}
	
	return false;
}

#endif // BUILD_AUDIO_ANALYSIS