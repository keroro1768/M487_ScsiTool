# M487 Flash via GDB load
# Start OpenOCD in background, then load via GDB
echo "Starting OpenOCD in background..."
spawn {*}[exec "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat" -s "D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD\scripts" -f "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg"]

# Wait for GDB server to be ready
sleep 2

# Connect via GDB and load
set result [exec "C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\bin\arm-none-eabi-gdb.exe" -ex "target remote localhost:3333" -ex "load D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.bin" -ex "detach" -ex "quit"]

echo "Result: $result"

shutdown