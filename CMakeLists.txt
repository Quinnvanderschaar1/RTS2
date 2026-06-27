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
    src/ProcessingThreads.cpp
    src/TimingLogger.cpp
    src/Transceiver.cpp
    src/Globals.cpp
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



# Install executable
install(TARGETS conferencing
    RUNTIME DESTINATION bin
)

# Package metadata
set(CPACK_PACKAGE_NAME "conferencing")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_CONTACT "quinnvanderschaar@live.nl")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ conferencing application")

# Package formats
set(CPACK_GENERATOR "DEB;TGZ;ZIP")

# Debian metadata
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Quinn")

if(PORTAUDIO_FOUND)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libportaudio2, libstdc++6")
else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libstdc++6")
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/man/conferencing.1
    DESTINATION share/man/man1
)


include(CPack)