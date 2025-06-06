#pragma once

#include "ofMain.h"
#include "ofxImGui.h"

#include "seam/editor.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void exit();
		void update();
		void draw();
		void drawOutput(ofEventArgs& args);
		void audioIn(ofSoundBuffer& input);

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);

private:
		bool showGui = true;
		ofxImGui::Gui gui;
		seam::Editor seamEditor;

		ofSoundStreamSettings soundSettings;
		ofSoundStream soundStream;
};
