# Created by and for Qt Creator This file was created for editing the project sources only.
# You may attempt to use it for building too, by modifying this file here.

cmake_minimum_required(VERSION 3.5)
project(miniMidiOut)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_library(PORTAUDIO_LIBRARIES NAMES portaudio)

include_directories(
    include
    include/device
)

set (SRCS
    include/device/keyboard.h
    include/device/midi.h
    include/device.h
    include/globals.h
    include/Modus.h
    include/noise.h
    include/player.h
    include/PseudoRandom.h
    include/time_tools.h
    include/voice.h
    include/MemoryLock.h
    include/realtime.h
    src/device/keyboard.cpp
    src/device/midi.cpp
    src/device.cpp
    src/globals.cpp
    src/main.cpp
    src/Modus.cpp
    src/noise.cpp
    src/player.cpp
    src/PseudoRandom.cpp
    src/time_tools.cpp
    src/voice.cpp
    src/realtime.cpp
)


add_executable(${CMAKE_PROJECT_NAME} ${SRCS})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${PORTAUDIO_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE ${PORTAUDIO_LIBRARIES})

configure_file(${CMAKE_SOURCE_DIR}/etc/default.conf ${CMAKE_BINARY_DIR}/default.conf COPYONLY)
