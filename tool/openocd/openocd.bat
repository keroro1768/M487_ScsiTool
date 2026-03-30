@echo off
set PATH=%~dp0..\OpenOCD;C:\msys64\mingw64\bin;%PATH%
"%~dp0..\OpenOCD\bin\openocd.exe" %*
