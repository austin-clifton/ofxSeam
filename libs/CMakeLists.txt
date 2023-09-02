# TODO pre-configure some options for Gist and BTrack so fftw and libsamplerate can be found without further configuration.

#target_include_directories(seam PRIVATE ...)

# Don't build Gist and BTrack tests.
set(BUILD_TESTS OFF CACHE BOOL "Enable Gist and BTrack tests")

add_subdirectory(capnproto)

# add_subdirectory(fftw-3.3.10)
add_subdirectory(libsamplerate)
add_subdirectory(Gist)
# add_subdirectory(BTrack)

# TODO handle nativefiledialog