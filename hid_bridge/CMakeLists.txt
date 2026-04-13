cmake_minimum_required(VERSION 3.15)
project(M487HidTool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(SOURCES
    src/hid_device.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# Windows SDK HID/setupapi libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    setupapi
    hid
    cfgmgr32
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /EHsc
        /utf-8
        /W3
    )
    # Suppress warnings from SDK headers
    target_compile_options(${PROJECT_NAME} PRIVATE
        /wd4201  # nonstandard extension used: nameless struct/union
        /wd4214  # nonstandard extension used: bit field types other than int
        /wd4324  # 'xxx': structure was padded due to __declspec(align())
    )
endif()
