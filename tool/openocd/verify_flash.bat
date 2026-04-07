@echo off
:: Start OpenOCD in background
start /b cmd /c "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts -f D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg"

:: Wait for server
timeout /t 3 /nobreak >nul

:: Read first 32 bytes of flash at different addresses
echo Reading flash at different addresses...
"C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin\arm-none-eabi-gdb.exe" -ex "target remote localhost:3333" -ex "set confirm off" -ex "echo === Flash at 0x0 ===" -ex "x/32xb 0x0" -ex "echo === Flash at 0x00100000 (LDROM) ===" -ex "x/32xb 0x00100000" -ex "echo === SRAM at 0x20000000 ===" -ex "x/32xb 0x20000000" -ex "detach" -ex "quit"

:: Cleanup
taskkill /F /IM openocd.exe 2>nul