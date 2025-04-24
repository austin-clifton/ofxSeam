#include "ofApp.h"

#include "nodes/fireflies.h"
#include "nodes/forceGrid.h"

//--------------------------------------------------------------
void ofApp::setup(){
	// globally disable arbitrary texture sizes for now.
	// ImGUI seems to have trouble drawing arb rects...?
	ofDisableArbTex();
	gui.setup(nullptr, false);

	soundStream.printDeviceList();

	// If you want to analyze audio, choose your audio drivers here.
	auto devices = soundStream.getMatchingDevices("Solid State", 2, 2, ofSoundDevice::Api::MS_ASIO);
	
	if (devices.size()) {
		auto& dv = devices[0];
		soundSettings.setInDevice(dv);
		soundSettings.setOutDevice(dv);
		soundSettings.setApi(dv.api);
		soundSettings.numOutputChannels = dv.outputChannels;
		soundSettings.numInputChannels = dv.inputChannels;
		soundSettings.sampleRate = 44100;
	} else {
		ofLogError("Requested audio device not found");
		// assert(0);
	}

#if BUILD_AUDIO_ANALYSIS
	soundSettings.setInListener(this);
	soundSettings.bufferSize = 512;
	soundStream.setup(soundSettings);
#endif

	// Add your custom Seam nodes here.
	seam::EventNodeFactory* factory = seamEditor.GetFactory();
	factory->Register(factory->MakeCreate<seam::nodes::Fireflies>());
	factory->Register(factory->MakeCreate<seam::nodes::ForceGrid>());

	seamEditor.Setup(&soundSettings);
}

void ofApp::exit() {
	soundStream.close();
}

//--------------------------------------------------------------
void ofApp::update(){
	seamEditor.Update();
}

//--------------------------------------------------------------
void ofApp::drawOutput(ofEventArgs& args){
	glfwWindowHint(GLFW_AUTO_ICONIFY, 0);
	seamEditor.DrawSelectedNode();
}

void ofApp::draw() {
	seamEditor.Draw();

	if (showGui) {
		gui.begin();

		seamEditor.GuiDraw();

		gui.end();
		gui.draw();
	}
}

void ofApp::audioIn(ofSoundBuffer& input) {
	seamEditor.ProcessAudio(input);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == 'g') {
		showGui = !showGui;
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
	seamEditor.OnWindowResized(w, h);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
