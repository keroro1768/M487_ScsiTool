# M487 Firmware Flash - source numicroM4.cfg

# HLA adapter for Nu-Link first-gen
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd

# Source numicroM4.cfg which defines swj_newdap
source [find target/numicroM4.cfg]

# Override chipname and DAP ID for M487
set CHIPNAME M487
set CPUDAPID 0x2BA01477

# Add flash bank for M487 APROM (512KB @ 0x0)
# The sourced cfg already creates banks, but let's add ours
# numicro flash driver: base address 0x0, 512KB
flash bank M487.aprom numicro 0x00000000 0x80000 0 0 M487.cpu

adapter speed 4000

# Flash programming
init
halt

echo "Erasing 512KB at 0x0..."
flash erase_address 0x00000000 0x80000

echo "Programming..."
program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0 verify reset exit