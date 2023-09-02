cmake_minimum_required(VERSION 3.10)

project(seam)

set (CMAKE_CXX_STANDARD 17)

# option(OPENFRAMEWORKS_LIB_PATH "The path to the built OpenFrameworks library you want to link" "/c/path/to/openframeworks-lib/")
find_library(LIB_OPENFRAMEWORKS NAMES openframeworks)

add_subdirectory(libs)
add_subdirectory(src)

# add_executable(seam )