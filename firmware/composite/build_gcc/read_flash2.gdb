target remote localhost:3333
# Read flash at APROM alias 0x00000000 (remapped to 0x10000000)
# This is where vector table should be
x/8x 0x00000000
# Also check SRAM
x/8x 0x20000000
detach
