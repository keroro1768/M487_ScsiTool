set pagination off
set confirm off

target remote localhost:3333

# Halt and reset
monitor reset halt

# Load firmware to APROM flash at 0x00000000
# GDB treats address 0x00000000 as the APROM (maps to 0x10000000 internally)
echo === Loading firmware to APROM flash ===\n
load

echo === Registers ===\n
info registers sp pc

detach
quit
