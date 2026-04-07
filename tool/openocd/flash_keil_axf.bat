@echo off
:: Flash Keil AXF (ELF) to M487 via GDB
start /b cmd /c "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts -f D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg"

timeout /t 3 /nobreak >nul

:: Load Keil AXF via GDB
"C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin\arm-none-eabi-gdb.exe" -ex "file D:/AiWorkSpace/KM/M480BSP/SampleCode/StdDriver/HSUSBD_Mass_Storage_ShortPacket/KEIL/obj/HSUSBD_Mass_Storage_ShortPacket.axf" -ex "set confirm off" -ex "target extended-remote localhost:3333" -ex "mon halt" -ex "mon reset halt" -ex "load" -ex "mon reset run" -ex "detach" -ex "quit"

taskkill /F /IM openocd.exe 2>nul

echo Done!