set pagination off
set confirm off
target remote localhost:3333

# Read FMC registers to check if FMC is accessible
echo === FMC ISP Control @ 0x400C0000 ===\n
x/4x 0x400C0000

echo === FMC ISP CMD @ 0x400C0004 ===\n
x/4x 0x400C0004

echo === FMC ISP ADDR @ 0x400C0008 ===\n
x/4x 0x400C0008

echo === FMC ISP DATA @ 0x400C000C ===\n
x/4x 0x400C000C

echo === FMC ISP TRIG @ 0x400C0010 ===\n
x/4x 0x400C0010

echo === FMC ISP CRSR ADDR (0x400C0200) ===\n
x/4x 0x400C0200

detach
quit
