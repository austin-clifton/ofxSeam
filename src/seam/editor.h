#pragma once

#include <vector>


#include "INIReader.h"

#include "seam/factory.h"
#include "seam/seamGraph.h"
#include "seam/include.h"
#include "seam/pins/push.h"
#include "seam/framePool.h"

namespace seam {

	using namespace nodes;
	using namespace pins;

	/// node graph editor manages the event nodes and connections that make up a seam scene
	class Editor {
	public:
		const static char* CONFIG_FILE_NAME;

		/// to be called by ofApp::setup()
		void Setup(ofSoundStreamSettings* soundSettings);

		~Editor();

		/// to be called by ofApp::draw()
		void Draw();

		void GuiDraw();

		void DrawSelectedNode();

		/// to be called by ofApp::update()
		void Update();

		void OnWindowResized(int w, int h);

		void ProcessAudio(ofSoundBuffer& buffer);

		seam::EventNodeFactory* GetFactory() { return graph.GetFactory(); }
	
	private:
		void ResizeWindow(int32_t width, int32_t height);

		void NewGraph();
		void SaveGraph(const std::string_view filename, const std::vector<INode*>& nodesToSave);
		void LoadGraph(const std::string_view filename);
		
		bool Connect(PinInput* pinIn, PinOutput* pinOut);

		bool Disconnect(PinInput* pinIn, PinOutput* pinOut);

		void GuiDrawPopups();

		ax::NodeEditor::EditorContext* nodeEditorContext = nullptr;

		// List of all links between pins, mostly for GUI display + interactions
		std::vector<SeamGraph::Link> links;

		// book keeping for GUI interactions
		Pin* newLinkPin;
		/// @brief Node selected with a left click (will have its node properties menu opened)
		INode* selectedNode = nullptr;
		/// @brief Node selected with a right click (will have a context menu popup opened)
		INode* selectedContextMenuNode = nullptr;
		/// @brief Last visual node selected in the GUI. 
		/// Its display FBO will be rendered to the GUI window.
		INode* lastSelectedVisualNode = nullptr;

		bool showCreateDialog = false;
		bool showWindowResize = false;
		glm::ivec2 windowSize;

		std::string loadedFile;

		INIReader iniReader = INIReader(CONFIG_FILE_NAME);

		SeamGraph graph;
	};
}