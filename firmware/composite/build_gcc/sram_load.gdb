set pagination off
set confirm off

target remote localhost:3333

# Halt CPU first
monitor halt

# Load firmware to SRAM at 0x20000000
echo === Loading firmware to SRAM 0x20000000 ===\n
load firmware.elf

echo === Registers after load ===\n
info registers sp pc

# Set PC to entry point (startup code)
echo === Set PC to Reset_Handler ===\n
break Reset_Handler
monitor reg pc 0x20001D01

echo === Continue from SRAM ===\n
continue

detach
quit
