cmake_minimum_required(VERSION 3.20)

# Toolchain - must set CMAKE_SYSTEM_NAME BEFORE project()
set(TOOLCHAIN "C:/Users/rinry/Tool/xpack-arm-none-eabi-gcc-15.2.1-1.1")
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_C_COMPILER "${TOOLCHAIN}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_AR "${TOOLCHAIN}/bin/arm-none-eabi-ar.exe")
set(CMAKE_RANLIB "${TOOLCHAIN}/bin/arm-none-eabi-ranlib.exe")
set(CMAKE_OBJCOPY "${TOOLCHAIN}/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_SIZE "${TOOLCHAIN}/bin/arm-none-eabi-size.exe")

project(M487_USB_Composite_GCC C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Suppress cmake_working_directory warnings
set(CMAKE_SUPPRESS_REGENERATION TRUE)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# MCU flags
set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16")

# Source files (absolute paths)
set(PROJ_DIR "D:/AiWorkSpace/M487_ScsiTool/firmware/composite")
set(BSP_DIR  "D:/AiWorkSpace/KM/M480BSP")

set(PROJ_SRCS
    ${PROJ_DIR}/main.c
    ${PROJ_DIR}/hid_i2c.c
    ${PROJ_DIR}/usb_descriptors.c
    D:/AiWorkSpace/M487_ScsiTool/firmware/i2c_control.c
)

set(BSP_SRCS
    ${BSP_DIR}/Library/StdDriver/src/hsusbd.c
    ${BSP_DIR}/Library/StdDriver/src/clk.c
    ${BSP_DIR}/Library/StdDriver/src/gpio.c
    ${BSP_DIR}/Library/StdDriver/src/sys.c
    ${BSP_DIR}/Library/StdDriver/src/pdma.c
    ${BSP_DIR}/Library/StdDriver/src/fmc.c
    ${BSP_DIR}/Library/StdDriver/src/usbd.c
    ${BSP_DIR}/Library/StdDriver/src/retarget.c
    ${BSP_DIR}/Library/StdDriver/src/uart.c
)

set(STARTUP_SRC
    ${BSP_DIR}/Library/Device/Nuvoton/M480/Source/GCC/startup_M480.S
)

set(LINKER_SCRIPT
    ${BSP_DIR}/Library/Device/Nuvoton/M480/Source/GCC/gcc_arm.ld
)

# Include directories
set(INCLUDE_DIRS
    ${BSP_DIR}/Library/CMSIS/Include
    ${BSP_DIR}/Library/Device/Nuvoton/M480/Include
    ${BSP_DIR}/Library/StdDriver/inc
    ${PROJ_DIR}
    D:/AiWorkSpace/M487_ScsiTool/firmware
)

set(CMAKE_C_FLAGS_INIT "${MCU_FLAGS} -O2 -Wall -fdata-sections -ffunction-sections -fno-strict-aliasing -fno-builtin -DUSE_HXTAL=12000000 -D__ARM_ARCH_7M__=1")
set(CMAKE_ASM_FLAGS_INIT "${MCU_FLAGS} -D__ASSEMBLY__")

foreach(dir IN LISTS INCLUDE_DIRS)
    set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -I${dir}")
    set(CMAKE_ASM_FLAGS_INIT "${CMAKE_ASM_FLAGS_INIT} -I${dir}")
endforeach()

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "${MCU_FLAGS} -T${LINKER_SCRIPT} -Wl,--gc-sections -Wl,-Map=${CMAKE_BINARY_DIR}/firmware.map")

add_executable(firmware.elf
    ${PROJ_SRCS}
    ${BSP_SRCS}
    ${STARTUP_SRC}
)

# Generate binary and hex
add_custom_command(TARGET firmware.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary firmware.elf firmware.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex firmware.elf firmware.hex
    COMMAND ${CMAKE_SIZE} firmware.elf
    COMMENT "Generating firmware.bin and firmware.hex"
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_INIT}")
set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS_INIT}")
