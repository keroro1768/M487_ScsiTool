cmake_minimum_required(VERSION 3.15)
project(M487USBStorage CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source files
set(SOURCES
    src/usb.cpp
    src/scsi.cpp
    src/device.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Windows SDK libraries
target_link_libraries(${PROJECT_NAME}
    setupapi
    winusb
    cfgmgr32
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

# MSVC flags
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /EHsc
        /utf-8
        /W0
    )
endif()
