@echo off
set PATH=C:\msys64\mingw64\bin;%PATH%
set PATH=%PATH%;D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin
"D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin\openocd.exe" %*
