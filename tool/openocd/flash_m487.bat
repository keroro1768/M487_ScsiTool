@echo off
set OPENOCD_PATH=D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat
set SCRIPT_PATH=-s D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts
set CFG=-f D:\AiWorkSpace\M487_ScsiTool\tool\openocd\m487_flash.cfg

:: Flash firmware using inline commands after config
"%OPENOCD_PATH%" %SCRIPT_PATH% %CFG% -c "init" -c "halt" -c "flash erase_address 0x00000000 0x80000" -c "program D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin 0x0 verify reset exit"