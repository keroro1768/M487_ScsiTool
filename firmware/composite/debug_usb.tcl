init
reset halt
poll
echo "=== USB Registers ==="
# HSUSBD base = 0x40090000
# Check USB interrupt enable
mdw 0x40090000 8
# Check USB status
mdw 0x40090100 8
# Check endpoint registers
mdw 0x40090200 16
echo "=== FMC Registers ==="
mdw 0x4000E000 8
echo "=== PC ==="
reg pc
echo "=== SRAM at 0x20000000 ==="
mdw 0x20000000 8
shutdown
