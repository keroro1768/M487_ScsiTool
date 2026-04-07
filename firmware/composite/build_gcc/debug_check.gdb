set pagination off
set confirm off

# Connect to OpenOCD
target remote localhost:3333

# Reset and halt
monitor reset halt

# Show current PC
info registers pc

# Read PC value
monitor reg pc

# Check current instruction at PC
x/4i $pc

# Disconnect
detach
quit
