target remote localhost:3333
# Read first 16 words of APROM base (0x00000000 = 0x10000000 remapped)
# These should be vector table
x/16x 0x0
detach
