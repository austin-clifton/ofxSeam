target_sources(seam PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/editor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/factory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/shader-utils.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/imgui-utils/properties.cpp
    
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/add-store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/compute-particles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/cos.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/feedback.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/fireflies.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/force-grid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/gate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/gist-audio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/i-node.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/markov.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/midi-in.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/noise.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/notes-printer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/percussive-trigger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/range.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/saw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/step.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/texgen-perlin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/timer.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/pins/pin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pins/push.cpp
)

target_sources(seam PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/editor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/factory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/flags-helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/frame-pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/hash.h
    ${CMAKE_CURRENT_SOURCE_DIR}/notes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ring-buffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/shader-utils.h

    ${CMAKE_CURRENT_SOURCE_DIR}/containers/octree.h

    ${CMAKE_CURRENT_SOURCE_DIR}/imgui-utils/properties.h

    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/add-store.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/compute-particles.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/cos.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/feedback.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/fireflies.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/force-grid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/gate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/gist-audio.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/i-node.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/markov.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/midi-in.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/noise.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/notes-printer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/percussive-trigger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/range.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/saw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/step.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/texgen-perlin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/nodes/timer.h

    ${CMAKE_CURRENT_SOURCE_DIR}/pins/pin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/pins/push.h
)