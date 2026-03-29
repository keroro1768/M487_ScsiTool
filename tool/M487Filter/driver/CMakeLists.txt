#
# CMakeLists.txt - Build M487Filter.sys via CMake
#
# NOTE: This is a reference CMake file for IDE/solution generation.
# Actual driver compilation still requires WDK build environment.
# Use: cmake -G "Visual Studio 17" -T WindowsDriver -S . -B build
#

cmake_minimum_required(VERSION 3.20)

project(M487Filter LANGUAGES C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Driver source files
#
set(DRIVER_SOURCES
    M487Filter.c
    M487FilterDevice.c
    M487FilterWmi.c
    M487FilterIoctl.c
    M487FilterRingBuffer.c
    M487FilterIntercept.c
    M487FilterEtw.c
    M487Filter.rc
)

#
# Public include directories (for IDE purposes)
#
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

#
# Build the driver
#
if(WIN32)
    add_library(M487Filter SHARED ${DRIVER_SOURCES})

    #
    # Note: WDK build rules are NOT directly handled by CMake.
    # This CMake file is primarily for IDE integration and
    # solution/project generation. Actual .sys compilation
    # requires the WDK build environment (build.exe or MSBuild
    # with WDK targets).
    #
    # To build from WDK command prompt:
    #   build -g -km  (for KMDF driver)
    #
    set_target_properties(M487Filter PROPERTIES
        OUTPUT_NAME "M487Filter"
        PREFIX ""
        SUFFIX ".sys"
    )
endif()
