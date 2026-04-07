set pagination off
set confirm off

target remote localhost:3333

# Try to load directly without halting first
# Use the ELF file's load address (0x00000000 -> maps to APROM)
echo === Loading firmware ===\n
load

echo \n=== Verify: Read back first 4 words at flash base ===\n
x/4x 0x10000000

echo \n=== Registers ===\n
info registers

detach
quit
