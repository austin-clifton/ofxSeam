add_subdirectory(seam)

add_executable(seam
    main.cpp
    ofApp.cpp
    ofApp.h
    ${seam_sources}
    ${seam_headers}

)

target_link_libraries(seam ${OPENFRAMEWORKS_LIB_PATH} capnp kj)
