@echo off
cd /d "C:\Users\rinry\Tool\openocd\OpenOCD-20260302-0.12.0\bin"
openocd.exe -s "C:\Users\rinry\Tool\openocd\OpenOCD-20260302-0.12.0\share\openocd\scripts" -f interface/cmsis-dap.cfg -f target/numicro_m4.cfg -c "init" -c "targets" -c "exit"
