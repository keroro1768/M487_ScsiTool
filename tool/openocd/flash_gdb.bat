@echo off
echo Starting OpenOCD server in background...
start /b "OpenOCD" cmd /c "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts -f D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg"

echo Waiting for GDB server to start...
timeout /t 3 /nobreak >nul

echo Loading firmware via GDB...
"C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin\arm-none-eabi-gdb.exe" -ex "set confirm off" -ex "target remote localhost:3333" -ex "load D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin" -ex "detach" -ex "quit"

echo Done!
pause