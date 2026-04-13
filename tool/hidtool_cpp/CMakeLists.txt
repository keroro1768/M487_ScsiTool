cmake_minimum_required(VERSION 3.15)
project(M487HidTool CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Library
add_library(m487hid STATIC
    src/m487_hid_device.cpp
)

target_include_directories(m487hid PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(m487hid PUBLIC
    setupapi
    hid
    cfgmgr32
)

# CLI test executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE m487hid)

if(MSVC)
    target_compile_options(m487hid PRIVATE /EHsc /utf-8 /W3)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /utf-8 /W3)
elseif(MINGW)
    target_compile_options(m487hid PRIVATE -Wall -Wextra)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
