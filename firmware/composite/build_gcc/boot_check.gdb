target remote localhost:3333
monitor reset halt
# Wait for boot ROM to run and remap FMC, then halt
# The boot ROM should jump to flash (if valid) within ~100ms
# We'll just check: if pc is in ROM range (0x10000000-0x10100000) = no flash boot
# If pc is in flash range (0x00000000-0x00100000) = flash boot
# If pc is in SRAM (0x20000000+) = SRAM boot
monitor reg pc
monitor reg xpsr
detach
