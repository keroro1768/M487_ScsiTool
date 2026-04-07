# M487 complete flash script - correct flash size, no verify
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd

# Setup SWD DAP
swd newdap M487 cpu -irlen 4 -expected-id 0x2BA01477 -ignore-version
dap create M487.dap -chain-position M487.cpu

# Create target
target create M487.cpu cortex_m -dap M487.dap
M487.cpu configure -work-area-phys 0x20000000 -work-area-size 0x4000

# Flash bank - numicro driver - let it auto-detect size
flash bank M487.flash numicro 0x00000000 0 0 0 M487.cpu

adapter speed 4000

# Programming sequence - no verify
init
halt
flash erase_address 0x00000000 0x20000
program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0 reset exit