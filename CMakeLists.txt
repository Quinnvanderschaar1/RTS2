cmake_minimum_required(VERSION 3.16)

project(conferencing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

option(USE_SIMULATION "Build with software simulation instead of hardware audio/GPIO" ON)

pkg_check_modules(PORTAUDIO QUIET portaudio-2.0)

if(NOT USE_SIMULATION AND NOT PORTAUDIO_FOUND)
    message(FATAL_ERROR "PortAudio not found and USE_SIMULATION=OFF. Install portaudio or build with -DUSE_SIMULATION=ON.")
endif()

set(SRC_FILES
    src/main.cpp
    src/UserInterface.cpp
    src/AudioFifo.cpp
    src/AudioMixer.cpp
    src/AudioProcessing.cpp
    src/UdpSender.cpp
    src/UdpReceiver.cpp
    src/SimulatedAudio.cpp
    src/SimulatedUI.cpp
    src/TimingLogger.cpp
    src/Transceiver.cpp
)

if(NOT USE_SIMULATION OR PORTAUDIO_FOUND)
    list(APPEND SRC_FILES
        src/AudioRecorder.cpp
        src/AudioPlayer.cpp
    )
endif()

add_executable(conferencing ${SRC_FILES})

if(PORTAUDIO_FOUND)
    target_include_directories(conferencing PRIVATE
        include
        ${PORTAUDIO_INCLUDE_DIRS}
    )

    target_link_libraries(conferencing PRIVATE
        ${PORTAUDIO_LIBRARIES}
        Threads::Threads
    )

    target_compile_options(conferencing PRIVATE
        ${PORTAUDIO_CFLAGS_OTHER}
    )
else()
    target_include_directories(conferencing PRIVATE include)
    target_link_libraries(conferencing PRIVATE Threads::Threads)
endif()
