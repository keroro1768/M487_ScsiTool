@echo off
set OPENOCD_PATH=D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat
set SCRIPT_PATH=-s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts

:: Simple init to check IDCODE
"%OPENOCD_PATH%" %SCRIPT_PATH% -c "adapter driver hla" -c "hla layout nulink" -c "hla vid_pid 0x0416 0x511C" -c "transport select swd" -c "swd newdap M487 cpu -irlen 4 -expected-id 0x2BA01477" -c "dap create M487.dap -chain-position M487.cpu" -c "target create M487.cpu cortex_m -dap M487.dap" -c "adapter speed 4000" -c "init" -c "echo Target initialized" -c "shutdown"