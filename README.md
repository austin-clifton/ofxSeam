# seam

Seam is a pin-based node editor for openFrameworks built using:
- [openFrameworks](https://openframeworks.cc/)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [the node editor extension for ImGui](https://github.com/thedmd/imgui-node-editor)

It is heavily WIP, and currently too barren of features to be effectively used yet. But, the scaffolding is becoming more stable!

## Design Goals

Seam's design goals include:
- build a system like TouchDesigner, with audio and musical input as a primary focus
- use open source tools and architect code that can be built on and extended easily
- easy integration of external signals over pipes, MIDI input, etc.
- make audio-reactive visual systems easier to build and iterate
- make adding new nodes of various complexity easy
- make shaders easy to plug with different inputs 
- efficient update and draw loops, a pull based update/draw system where visible nodes determine what needs to update
- end executables that you can send to your friends, and have them hook their beats up, or _eventually_, even use for real-time visual performances.

## Roadmap

A shorter term roadmap for seam includes:
- node deletion 
- prettify it (use a better font)
- serialize and deserialize node graphs to save and load "scene graphs" (will probably use [capnp](https://capnproto.org/capnp-tool.html) for this)
- multichannel pin connections with mapping options
- integrate more node types:
    - feedback
    - MIDI input
    - [SuperCollider](https://supercollider.github.io/) synth inputs
    - particle system node (probably would act like its own "scene", should probably be a compute shader)
    - the `seam::Cos` node could be a more generic LFO
    - generic shader node with variable input pins to map to shader inputs
    - ...many more
- smarter graph traversal
    - add ping pong FBO logic for nodes with feedback textures
    - smarter update logic (the flags handling updates over time in the `NodeFlags` enum aren't treated reasonably)
- screen management for multiple render targets
- TESTS, especially for the graph traversals, so I can trust the code
- a basic expression parser, [cparse](https://github.com/cparse/cparse) looks neat

## Code Layout

The C++ source for seam is in the `src/seam/` directory.

The bulk of the architecture (so far) is in these files in `src/seam/`:
- `event-nodes/event-node.h` is an abstract virtual base class for all event system nodes; the implementation file handles some common functionality
- `factory.h` describes the `EventNodeFactory` which holds generators which contain metadata about event node types, and can create them, for GUI and eventually deserialization purposes
- `editor.h` describes the interface for adding and removing nodes and links, drawing the node editor GUI, and is also currently responsible for calculating update and draw orders
    - following TouchDesigner's "pull-based" eventing system, I attempt to only request updates on "dirty" nodes when the nodes need to update for drawing's purpose

Two samples of `IEventNode` implementations are in `event-nodes/` directory, `texgen-perlin.h/cpp` and `cos.h/cpp`.

I have started an interface for drawing pins and other properties in the properties editor (top right hand corner when a node is selected) in `imgui-utils/properties.h` and cpp.

`pin.h` describes the pin types that I plan to implement; I've only fully implemented the GUI drawing code for float and int pins so far.

## Building

It is currently difficult to build seam since I have not uploaded the updated ImGui node editor source code I am using.

I am also using a [modified branch of the openFrameworks source](https://github.com/austin-clifton/openFrameworks) which supports c++17 on Windows.

I will update this section when building seam is easier (probably when I upload the modified imgui sources).