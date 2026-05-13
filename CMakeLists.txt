cmake_minimum_required(VERSION 3.16)

project(conferencing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_executable(conferencing
    src/main.cpp
    src/UserInterface.cpp
    src/AudioFifo.cpp
    src/AudioRecorder.cpp
    src/AudioPlayer.cpp
    src/UdpSender.cpp
    src/UdpReceiver.cpp
    src/AudioMixer.cpp
    src/AudioProcessing.cpp
)

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