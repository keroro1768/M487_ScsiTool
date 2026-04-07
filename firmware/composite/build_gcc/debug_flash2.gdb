set pagination off
set confirm off

target remote localhost:3333

# Halt without reset
monitor halt

# Read flash at 0x00000000 (vector table start)
echo === Flash @ 0x00000000 ===\n
x/16x 0x00000000

echo \n=== Flash @ 0x00000004 (reset vector) ===\n
x/1x 0x00000004

echo \n=== Current Registers ===\n
info registers

detach
quit
