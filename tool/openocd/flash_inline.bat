@echo off
set OPENOCD_PATH=D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat
set SCRIPT_PATH=-s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts

:: Inline configuration with numicro flash driver
"%OPENOCD_PATH%" %SCRIPT_PATH% -c "adapter driver hla" -c "hla layout nulink" -c "hla vid_pid 0x0416 0x511C" -c "transport select swd" -c "swd newdap M487 cpu -irlen 4 -expected-id 0x2BA01477" -c "dap create M487.dap -chain-position M487.cpu" -c "target create M487.cpu cortex_m -dap M487.dap" -c "M487.cpu configure -work-area-phys 0x20000000 -work-area-size 0x4000" -c "flash bank M487.flash numicro 0x00000000 0 0 0 M487.cpu" -c "adapter speed 4000" -c "init" -c "halt" -c "flash erase_address 0x00000000 0x80000" -c "program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0 verify reset exit"