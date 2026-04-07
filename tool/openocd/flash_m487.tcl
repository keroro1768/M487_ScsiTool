# M487 Firmware Flash Script
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f flash_m487.tcl

init
wait_halt
reset halt

# Flash firmware
flash write_image erase D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0

# Verify
reset run
shutdown