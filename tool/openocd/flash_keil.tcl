# M487 Flash Keil Binary
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd

# Setup SWD DAP - bypass IDCODE check
swd newdap M487 cpu -irlen 4 -expected-id 0x2BA01477 -ignore-version
dap create M487.dap -chain-position M487.cpu

# Create target
target create M487.cpu cortex_m -dap M487.dap
M487.cpu configure -work-area-phys 0x20000000 -work-area-size 0x4000

# Flash bank - numicro driver
flash bank M487.flash numicro 0x00000000 0 0 0 M487.cpu

adapter speed 4000

# Programming - program command handles erase internally
init
halt
program D:/AiWorkSpace/KM/M480BSP/SampleCode/StdDriver/HSUSBD_Mass_Storage_ShortPacket/KEIL/obj/HSUSBD_Mass_Storage_ShortPacket.bin 0x0 verify reset exit