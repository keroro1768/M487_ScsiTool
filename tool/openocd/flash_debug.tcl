# M487 Debug + Flash Script
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f flash_debug.tcl

init
halt

# List flash banks
echo "=== Flash Banks ==="
flash banks

# List flash drivers
echo "=== Flash List ==="
flash list

shutdown