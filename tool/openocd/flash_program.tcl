# M487 Firmware Flash using program command
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f flash_program.tcl

init
halt

# Use program command for generic flash programming
# Format: program <filename> <offset> [preverify] [verify] [reset] [exit]
program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0 verify reset exit