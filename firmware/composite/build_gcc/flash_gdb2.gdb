set pagination off
set confirm off
target remote localhost:3333
load D:/AiWorkSpace/M487_ScsiTool/firmware/composite/build_gcc/firmware.elf 0x10000000
echo === Verify flash @ 0x10000000 ===\n
x/4x 0x10000000
detach
quit
