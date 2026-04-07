# M487 Firmware Flash Script v2
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f flash_m487_v2.tcl

init
halt

# Initialize flash banks
flash init

# M487 internal flash: 512KB @ 0x0
# Set up FMC flash bank
# nuvoton m487 has 512KB APROM starting at 0x0
flash bank 0 nuvoton 0x00000000 0x80000 0 0 M487.cpu

# Flash the firmware
flash write_image erase D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0

# Verify
reset run
shutdown