target remote localhost:3333
monitor reset halt
# Read first 32 bytes of APROM (0x00000000 -> 0x10000000 after remap)
# Our firmware starts at offset 0x200 into APROM (after vector table)
x/16x 0x10000200
# Read USB descriptor location in our firmware (should have 0x04F3, 0x0732)
search 0x04F3
# Read CHIP_ID at FMC base
x/4x 0x50000000
detach
