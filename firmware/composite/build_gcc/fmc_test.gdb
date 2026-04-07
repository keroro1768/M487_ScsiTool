set pagination off
set confirm off

target remote localhost:3333

# Enable FMC ISP
set *((unsigned int*)0x400C0000) = 0x01
echo ISP enabled\n

# Read ISPDAT to confirm
x/x 0x400C000C

# Write a test word to SRAM (should persist after we read it back)
set *((unsigned int*)0x20000000) = 0xDEADBEEF
echo SRAM test write:\n
x/x 0x20000000

# Try to read from APROM (blank, should return 0xFFFFFFFF or 0)
echo Read APROM @ 0x10000000:\n
x/x 0x10000000

# Check FMC CID (should be 0x0 at reset)
echo FMC CID @ 0x400C00F0:\n
x/x 0x400C00F0

detach
quit
