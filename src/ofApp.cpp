#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	// globally disable arbitrary texture sizes for now.
	// ImGUI seems to have trouble drawing arb rects...?
	ofDisableArbTex();
	gui.setup(nullptr, false);

	ofSoundStreamSettings settings;

	// if you want to set the device id to be different than the default
	auto devices = soundStream.getDeviceList();
	// settings.device = devices[4];

	// you can also get devices for an specific api
	// auto devices = soundStream.getDevicesByApi(ofSoundDevice::Api::PULSE);
	// settings.device = devices[0];

	// or get the default device for an specific api:
	// settings.api = ofSoundDevice::Api::PULSE;

	// or by name
	/*
	auto devices = soundStream.getMatchingDevices("default");
	if (!devices.empty()) {
		settings.setInDevice(devices[0]);
	}
	*/

	settings.setInListener(this);
	settings.sampleRate = 44100;
	settings.numOutputChannels = 0;
	settings.numInputChannels = 1;
	settings.bufferSize = 512;
	settings.setApi(ofSoundDevice::Api::MS_DS);
	soundStream.setup(settings);

	seam_editor.Setup();
}

void ofApp::exit() {
	soundStream.close();
}

//--------------------------------------------------------------
void ofApp::update(){
	seam_editor.Update();
}

//--------------------------------------------------------------
void ofApp::drawOutput(ofEventArgs& args){
	glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
	seam_editor.DrawSelectedNode();
}

void ofApp::draw() {
	seam_editor.Draw();

	if (show_gui) {
		gui.begin();

		seam_editor.GuiDraw();

		gui.end();
		gui.draw();
	}
}

void ofApp::audioIn(ofSoundBuffer& input) {
	std::vector<float>* buffer = &input.getBuffer();

	seam::nodes::ProcessAudioParams params;
	params.buffer = &input.getBuffer();

	seam_editor.ProcessAudio(&params);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == 'g') {
		show_gui = !show_gui;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
