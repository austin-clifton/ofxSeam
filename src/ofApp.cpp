#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	// globally disable arbitrary texture sizes for now.
	// ImGUI seems to have trouble drawing arb rects...?
	ofDisableArbTex();
	gui.setup(nullptr, false);
	seam_editor.Setup();
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
