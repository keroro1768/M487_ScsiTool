target remote localhost:3333
monitor reset halt
# After reset halt, pc should be at ROM or SRAM
# Let's check what memory map is active
monitor reg pc
monitor reg xpsr
monitor reg msp
# Check SRAM content
x/16x 0x20000000
detach
