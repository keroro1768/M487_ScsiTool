# M487 Debug + Info Script v2
# Usage: openocd.bat -s ... -f nulink_m487_ice.cfg -f info2.tcl

init
halt

# Show target
echo "=== Target Names ==="
target names

# Examine memory at known addresses
echo "=== Flash Base (0x0) ==="
mdw 0x00000000 8

echo "=== SRAM (0x20000000) ==="
mdw 0x20000000 8

echo "=== FMC Registers ==="
mdw 0x40003100 8

shutdown