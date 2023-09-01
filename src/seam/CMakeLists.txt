set(seam_sources
    editor.cpp
    factory.cpp
    shader-utils.cpp

    imgui-utils/properties.cpp
    
    nodes/add-store.cpp
    nodes/compute-particles.cpp
    nodes/cos.cpp
    nodes/feedback.cpp
    nodes/fireflies.cpp
    nodes/force-grid.cpp
    nodes/gate.cpp
    nodes/gist-audio.cpp
    nodes/i-node.cpp
    nodes/markov.cpp
    nodes/midi-in.cpp
    nodes/noise.cpp
    nodes/notes-printer.cpp
    nodes/percussive-trigger.cpp
    nodes/range.cpp
    nodes/saw.cpp
    nodes/shader.cpp
    nodes/step.cpp
    nodes/texgen-perlin.cpp
    nodes/timer.cpp

    pins/pin.cpp
    pins/push.cpp

    PARENT_SCOPE
)

set(seam_headers
    editor.h
    factory.h
    flags-helper.h
    frame-pool.h
    hash.h
    notes.h
    ring-buffer.h
    shader-utils.h

    containers/octree.h

    imgui-utils/properties.h

    nodes/add-store.h
    nodes/compute-particles.h
    nodes/cos.h
    nodes/feedback.h
    nodes/fireflies.h
    nodes/force-grid.h
    nodes/gate.h
    nodes/gist-audio.h
    nodes/i-node.h
    nodes/markov.h
    nodes/midi-in.h
    nodes/noise.h
    nodes/notes-printer.h
    nodes/percussive-trigger.h
    nodes/range.h
    nodes/saw.h
    nodes/shader.h
    nodes/step.h
    nodes/texgen-perlin.h
    nodes/timer.h

    pins/pin.h
    pins/push.h

    PARENT_SCOPE
)