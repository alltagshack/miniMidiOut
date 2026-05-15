cmake_minimum_required(VERSION 3.5)
project(synth0815)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(PORTAUDIO_LIBRARY portaudio)
if(NOT PORTAUDIO_LIBRARY)
    message(FATAL_ERROR
        "PortAudio not found.\n"
        "Install the Debian package: portaudio19-dev.")
endif()

find_library(ASOUND_LIBRARY asound)
if(NOT ASOUND_LIBRARY)
    message(FATAL_ERROR
        "ALSA (libasound) not found.\n"
        "Install the Debian package: libasound-dev.")
endif()

include_directories(
    include
)

set (SRCS
    src/Program.cpp
    src/SynthApp.cpp
    src/Voice.cpp
)


add_executable(${CMAKE_PROJECT_NAME} ${SRCS})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${PORTAUDIO_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${PORTAUDIO_LIBRARY} ${ASOUND_LIBRARY})
