set pagination off
set confirm off

target remote localhost:3333

# Halt without reset (avoid Rcmd error)
halt

# Load firmware to flash at physical address 0x10000000
echo === Loading firmware to APROM @ 0x10000000 ===\n
load firmware.elf 0x10000000

echo \n=== Verify: Read back first 4 words of loaded flash ===\n
x/4x 0x10000000

echo \n=== Registers after load ===\n
info registers sp pc

detach
quit
