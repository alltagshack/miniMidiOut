cmake_minimum_required(VERSION 3.16)
project(miniMidiOut CXX)

find_library(PORTAUDIO_LIBRARY portaudio)
if(NOT PORTAUDIO_LIBRARY)
    message(FATAL_ERROR
        "PortAudio not found.\n"
        "Install the Debian package: portaudio19-dev.")
endif()

find_package(Threads QUIET)
if(NOT Threads_FOUND)
    message(FATAL_ERROR
        "POSIX threads library not found.\n"
        "Install the Debian package: libpthread-stubs0-dev.")
endif()

find_library(MATH_LIBRARY m)
if(NOT MATH_LIBRARY)
    message(FATAL_ERROR
        "Math library (libm) not found.\n"
        "Install the Debian package: libc6-dev (contains libm).")
endif()

set(SOURCES
    src/main.cpp
    src/modus.cpp
    src/pseudo_random.cpp
    src/time_tools.cpp
    src/voice.cpp
    src/noise.cpp
    src/device.cpp
    src/device/keyboard.cpp
    src/device/midi.cpp
    src/player.cpp
    src/globals.cpp)

add_executable(miniMidiOut ${SOURCES})
target_include_directories(miniMidiOut PRIVATE include)

target_compile_features(miniMidiOut PUBLIC cxx_std_17)

target_compile_definitions(miniMidiOut PRIVATE
  $<$<CONFIG:Debug>:MINIMIDIOUT_DEBUG>
  $<$<CONFIG:Release>:MINIMIDIOUT_RELEASE>)

target_compile_options(miniMidiOut PRIVATE -Wall -Wextra)
target_compile_options(miniMidiOut PRIVATE
  $<$<CONFIG:Debug>:-g>
  $<$<CONFIG:Release>:-O3>)


target_link_libraries(miniMidiOut
    ${PORTAUDIO_LIBRARY}
    ${CMAKE_THREAD_LIBS_INIT}
    ${MATH_LIBRARY})
