target remote localhost:3333
# Halt MCU
monitor reset halt
# Read FMC registers to understand current state
monitor reg pc
# Read FMC base register
# FMC base = 0x40000000 on M480 series
# Check FMC ISPCTL (0x40000010) - flash control
set {int}0x40000010 = 0x00000001
# ISP enable
set {int}0x40000000 = 0x00000001
# Write to flash at 0x10000000 - try direct memory write
set {int}0x10000000 = 0x20028000
set {int}0x10000004 = 0x000041B1
# Verify
x/2x 0x10000000
detach
