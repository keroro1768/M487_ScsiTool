set pagination off
set confirm off

target remote localhost:3333

# Read flash at 0x00000004 (reset vector - initial PC after reset)
echo === Reading reset vector at 0x00000004 ===\n
x/1x 0x00000004

echo === Reading first 8 words of vector table ===\n
x/8x 0x00000000

echo === Current PC ===\n
info registers pc

detach
quit
