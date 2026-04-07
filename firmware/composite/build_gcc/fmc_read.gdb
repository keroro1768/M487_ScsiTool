set pagination off
set confirm off
target remote localhost:3333

# Check FMC base address registers
set $fmc_base = 0x400C0000
echo === FMC ISPCTL @ 0x400C0000 ===\n
x/x 0x400C0000
echo === FMC ISP CMD @ 0x400C0004 ===\n
x/x 0x400C0004
echo === FMC ISPDAT @ 0x400C000C ===\n
x/x 0x400C000C

echo === Config @ 0x400C000C ===\n
x/x 0x400C00F8

detach
quit
