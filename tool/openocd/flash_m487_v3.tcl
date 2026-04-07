# M487 Firmware Flash Script v3
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f flash_m487_v3.tcl

init
halt

# M487 has 512KB APROM @ 0x0
# nuvoton driver auto-detects FMC, no need for flash bank command
# Just erase and write

# Erase and program flash
flash write_image erase D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0

reset run
shutdown