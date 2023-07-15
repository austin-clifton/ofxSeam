#include "ofMain.h"
#include "ofApp.h"

#if RUN_DOCTEST

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#else

//========================================================================
int main( ){
	ofGLFWWindowSettings settings;
	settings.setGLVersion(4, 5);
	settings.setSize(2560, 1440);
	ofCreateWindow(settings);

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp());

}

#endif // RUN_DOCTEST
