# M487 Firmware Flash - minimal approach
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f flash_min.tcl

init
reset halt

# M487 has 512KB flash @ 0x0
# Try erase address first
echo "Erasing flash..."
flash erase_address 0x00000000 0x80000

# Then program
echo "Programming..."
program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0 verify exit

shutdown