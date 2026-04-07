@echo off
:: Start OpenOCD in background
start /b cmd /c "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts -f D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg"

:: Wait for server
timeout /t 3 /nobreak >nul

:: Load firmware via GDB using ELF file (supports raw binary load)
echo Loading firmware via GDB...
"C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin\arm-none-eabi-gdb.exe" -ex "file D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.elf" -ex "set confirm off" -ex "target extended-remote localhost:3333" -ex "mon halt" -ex "load" -ex "mon reset run" -ex "detach" -ex "quit"

:: Cleanup
taskkill /F /IM openocd.exe 2>nul

echo Done!