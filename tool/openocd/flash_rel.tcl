# M487 Firmware Flash - reliable version
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f flash_rel.tcl

# Just init first to establish connection
init

# Then halt
halt

# Verify we're connected
echo "Connected to M487"

# Now erase flash
echo "Erasing 512KB at 0x0..."
flash erase_address 0x00000000 0x80000

# Program
echo "Programming firmware..."
program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0 verify reset exit