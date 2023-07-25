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

	// First set up the GUI window, it's always windowed.
	settings.setSize(1920, 1080);
	settings.monitor = 0;
	settings.resizable = true;
	settings.windowMode = ofWindowMode::OF_WINDOW;
	settings.title = "Seam GUI";
	auto gui_window = ofCreateWindow(settings);

	auto app = make_shared<ofApp>();

	// Query monitors AFTER openframeworks has initialized glfw
	int total_monitors;
	GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
	GLFWmonitor** monitors = glfwGetMonitors(&total_monitors);

	std::shared_ptr<ofAppBaseWindow> main_window;
	settings.shareContextWith = gui_window;
	settings.title = "Seam Output";

#if _DEBUG
	settings.setSize(2560, 1440);
	main_window = ofCreateWindow(settings);
#else
	// IMPROVEMENT: windowing behavior should be more configurable...

	// If there's more than one monitor, 
	// use the primary as the GUI screen and a secondary monitor for a fullscreen view of the selected node.
	if (total_monitors > 1) {
		int posx, posy;
		glfwGetMonitorPos(monitors[1], &posx, &posy);
		settings.setPosition(glm::vec2(posx, posy));
		settings.monitor = 1;
		settings.windowMode = ofWindowMode::OF_FULLSCREEN;
		main_window = ofCreateWindow(settings);

	} else {
		// There's only one window, keep everything windowed.
		settings.setSize(2560, 1440);
		main_window = ofCreateWindow(settings);
	}
#endif

	ofAddListener(main_window->events().draw, app.get(), &ofApp::drawOutput);

	ofRunApp(gui_window, app);
	ofRunMainLoop();
}

#endif // RUN_DOCTEST
