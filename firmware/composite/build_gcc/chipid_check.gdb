set pagination off
set confirm off

target remote localhost:3333

# Read CHIP_ID from M487
echo === Reading CHIP_ID via M487->FMC->CHIPID ===\n
# FMC_BA is at 0x400C0000, CHIP_ID is at 0x400C0100
# Read from FMC register
monitor mdw 0x400C0000 4
monitor mdw 0x400C0100 4

# Try reading via SWJ
echo === Reading SWJ IDCODE ===\n
monitor scan 0x08 0x0 0x0

echo === Current r0-r3 ===\n
info registers r0 r1 r2 r3

detach
quit
