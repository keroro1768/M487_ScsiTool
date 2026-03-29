cmake_minimum_required(VERSION 3.15)
project(M487FilterWmiApp)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(M487FilterWmiApp M487FilterWmiApp.c)

# Windows SDK / WMI
target_link_libraries(M487FilterWmiApp
    ole32
    oleaut32
    uuid
)

# Disable warnings for clean build
target_compile_options(M487FilterWmiApp PRIVATE
    /W4
    /EHsc
)

# ================================================
# hidlog - HID packet log viewer
# ================================================
add_executable(hidlog hidlog.c)
target_link_libraries(hidlog
    advapi32
)
target_compile_options(hidlog PRIVATE
    /W4
    /EHsc
)
set_target_properties(hidlog PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Runtime checks
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(M487FilterWmiApp PRIVATE /RTC1)
endif()

# Output directory
set_target_properties(M487FilterWmiApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
