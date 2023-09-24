#include "gist-audio.h"

using namespace seam;
using namespace seam::nodes;

namespace {

}

GistAudio::GistAudio() : INode("Gist Audio") {
	flags = (NodeFlags)(flags | NodeFlags::UPDATES_EVERY_FRAME);

	// gist = std::unique_ptr<Gist<float>>(new Gist<float>(512, 44100));
}

GistAudio::~GistAudio() {

}

void GistAudio::ProcessAudio(ofSoundBuffer& buffer) {
	/*
	gist->processAudioFrame(*params->buffer);
	
	UpdateSampleAcc(rms, gist->rootMeanSquare());
	UpdateSampleAcc(peak, gist->peakEnergy());
	UpdateSampleAcc(energyDiff, gist->energyDifference());
	UpdateSampleAcc(hfc, gist->highFrequencyContent());
	UpdateSampleAcc(spectralCentroid, gist->spectralCentroid());
	UpdateSampleAcc(spectralFlat, gist->spectralFlatness());

	printf("%f\n", gist->spectralCentroid());
	*/
}

void GistAudio::Update(UpdateParams* params) {
	if (rms.samples == 0) {
		return;
	}

	ClearSampler(rms);
	ClearSampler(peak);
	ClearSampler(energyDiff);
	ClearSampler(hfc);
	ClearSampler(spectralCentroid);
	ClearSampler(spectralFlat);
}

pins::PinInput* GistAudio::PinInputs(size_t& size) {
	size = 0;
	return nullptr;
}

pins::PinOutput* GistAudio::PinOutputs(size_t& size) {
	size = 0;
	return nullptr;
}

void GistAudio::GuiDrawNodeCenter() {
	DrawSampler(rms, "rms");
	DrawSampler(peak, "peak");
	DrawSampler(energyDiff, "energyDiff");
	DrawSampler(hfc, "hfc");
	DrawSampler(spectralCentroid, "spec centroid");
	DrawSampler(spectralFlat, "spec flat");
}

void GistAudio::UpdateSampleAcc(GistAudio::SampleAcc& sacc, float value) {
	sacc.acc += value;
	sacc.samples += 1;
}

void GistAudio::ClearSampler(GistAudio::SampleAcc& sacc) {
	sacc.avg = sacc.acc / sacc.samples;
	sacc.acc = 0.f;
	sacc.samples = 0;
}

void GistAudio::DrawSampler(GistAudio::SampleAcc sacc, const char* label) {
	ImGui::Text(label);
	ImGui::SameLine();
	ImGui::ProgressBar(sacc.avg);
}