add_executable(seam
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ofApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ofApp.h
    #${seam_sources}
    # ${seam_headers}

)

add_subdirectory(seam)

target_link_libraries(seam ${OPENFRAMEWORKS_LIB_PATH} kj capnp Gist)

target_include_directories(seam PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    # TODO include openframeworks directories via an option...
)