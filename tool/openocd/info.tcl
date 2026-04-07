# M487 Debug + Info Script
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f info.tcl

init
halt

# Show all available commands
echo "=== Target Info ==="
target names
echo "=== Current Target ==="
which

# Try to examine flash via memory map
echo "=== Memory Map ==="
mdw 0x00000000 4
mdw 0x1FFFF000 4

shutdown