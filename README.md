# ofxSeam

Seam is a GUI-based node graph editor [addon](https://openframeworks.cc/learning/01_basics/how_to_add_addon_to_project/) for [OpenFrameworks](https://openframeworks.cc/), designed for protoyping and building data-driven visual systems.

Seam uses C++17 so OpenFrameworks 0.12 is recommended. Windows Visual Studio and msys2 library dependencies are pre-built; if you are using Linux or Mac, dependencies will need to be built (see the dependencies section).

See the `example_usage` directory for a skeleton you can use to get started using Seam. The Seam Editor hooks into OpenFrameworks' `update()`, `draw()`, etc. functions in your `ofApp`.

Seam is currently missing many quality of life features and has some annoying bugs. Node creation and connections are functional and node graph files can be saved and loaded, but don't expect a friendly experience _yet_. See the Known Issues section for more info.

Seam's design goals include:
- extensible node-and-pin based node graph editor that lets you easily create and add your own Node logic.
- make audio-reactive visual systems easier to build and iterate.
- quick integration of external signals (pipes, MIDI and OSC input, audio analysis) into visual systems.
- the ability to construct multi-pass render systems in real time, without some of the implementation baggage that accompanies game engines.

The core of Seam is open source because Seam is built using many open source libraries, including:
- [ofxImGuiNodeEditor](https://github.com/austin-clifton/ofxImGuiNodeEditor), which itself is a mashup of [the original ImGui extension addon for OpenFrameworks](https://github.com/jvcleave/ofxImGui) and the [ImGui Node Editor extension](https://github.com/thedmd/imgui-node-editor).
- [Cap'n Proto](https://capnproto.org/) for file serialization (and likely over-the-wire serialization in the future).
- [ofxMidi](https://github.com/danomatika/ofxMidi) for handling MIDI input using Seam nodes.
- ...and more, this list will continue growing as more open source libraries are added.

License
-------

Seam's core Node Editor addon for OpenFrameworks is distributed under the [MIT License](https://en.wikipedia.org/wiki/MIT_License).

Installation
------------

If installing for MacOS or Linux, you will need to build additional dependencies yourself (listed below).

Dependencies
------------
- [capnp and kj](https://capnproto.org/) (they are built together)
- [nativefiledialog](https://github.com/mlabbe/nativefiledialog)

These are already compiled for Windows, if you are using Linux or Mac OS you will need to compile them yourself. Please PR your additions!

Compatibility
------------
Seam has been built and run using UCRT64 msys2 and Visual Studio 2019 using OpenFrameworks 0.12. It _should_ work fine on other platforms, but has not been tested, and dependencies will need to be built (see above).

Known issues
------------
#### GUI Bugs
- Clicking the top-left arrow in the Seam Editor will cause the editor to collapse and it won't re-expand; `imgui.ini` in your app's `bin/` directory will need to be deleted to restore the editor.

#### Visual Studio Install
The Visual Studio addon install via the Project Generator seems not to respect options in the `addon_config.mk` file. I am currently unsure if this is due to the addon or OpenFrameworks. To work around this, right click on your project in Visual Studio, go to Properties, and then in the Properties pane:
- Navigate to "Configuration Properties" --> "C++" --> "Additional Include Directories" and remove all sub-directories of `addons\ofxSeam\libs\capnp\includes`.
- Navigate to "Configuration Properties" --> "Linker" --> "Additional Input" and add the **full file paths** (dependent on your ofxSeam install path) for the following:
    - `ofxSeam/libs/capnp/lib/vs/debug/capnp.lib`
    - `ofxSeam/libs/capnp/lib/vs/debug/kj.lib`
    - `ofxSeam/libs/nfd/lib/vs/nfd_d.lib`
    - If you build for release instead of debug, you will need to use the corresponding release libraries instead.


Version history
------------

### Version 0.01 (5 Sep 2023):
Initial commit after moving from the original repo; moved Seam code to an ofx addon.
