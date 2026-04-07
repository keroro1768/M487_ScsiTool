set pagination off
set confirm off

target remote localhost:3333

# Reset but don't run
monitor reset halt

# Read flash at 0x00000000 (vector table start)
echo === Flash @ 0x00000000 (16 words) ===\n
x/16x 0x00000000

echo \n=== Flash @ 0x00000004 (reset vector) ===\n
x/1x 0x00000004

echo \n=== Read SP (MSP) at 0x00000000 ===\n
x/1x 0x00000000

echo \n=== Read PC reset value at 0x00000004 ===\n
x/1x 0x00000004

echo \n=== Current Registers ===\n
info registers

detach
quit
