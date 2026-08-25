cmake_minimum_required(VERSION 3.20)
project(ApriltagDetector LANGUAGES CXX)

# Project version
set(PROJECT_VERSION "1.0.0")

# Platform detection
if(WIN32)
    set(PLATFORM "windows")
    add_compile_definitions(PLATFORM_WINDOWS=1)
elif(APPLE)
    set(PLATFORM "macos")
    add_compile_definitions(PLATFORM_MACOS=1)
elif(UNIX)
    set(PLATFORM "linux")
    add_compile_definitions(PLATFORM_LINUX=1)
endif()

# Architecture detection
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x64")
    add_compile_definitions(ARCH_X64=1)
else()
    set(ARCH "x86")
endif()

# ARM detection
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
    set(ARCH "arm64")
    add_compile_definitions(ARCH_ARM64=1)
elif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    set(ARCH "arm")
    add_compile_definitions(ARCH_ARM32=1)
endif()

# Output build directory
set(BUILD_DIR "${CMAKE_BINARY_DIR}/${PLATFORM}_${ARCH}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BUILD_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BUILD_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BUILD_DIR}/lib)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(BUILD_TESTS "Build tests" ON)
option(BUILD_WEB_UI "Build web UI" ON)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/third_party
)

# Dependencies - we'll use system libraries where possible
# For AprilTag, we'll implement our own or use a submodule

# Common library
add_subdirectory(src/common)

# Applications
add_subdirectory(src/launcher)
add_subdirectory(src/camera_detector)
add_subdirectory(src/pose_finder)

if(BUILD_WEB_UI)
    add_subdirectory(src/web_ui)
endif()

# Install targets
install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/ DESTINATION config)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/scripts/ DESTINATION scripts)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/web/ DESTINATION web)

message(STATUS "Building for ${PLATFORM}_${ARCH}")
message(STATUS "Output directory: ${BUILD_DIR}")
