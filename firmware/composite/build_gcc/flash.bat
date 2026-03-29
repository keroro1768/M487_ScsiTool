@echo off
setlocal enabledelayedexpansion

:: ============================================================================
:: M487 Composite Firmware Flash Script
:: ============================================================================
:: Environment variables (override in Makefile.config or system env):
::   OPENOCD_ROOT  - OpenOCD installation root
::   OPENOCD_SCR   - OpenOCD scripts directory
::   FIRMWARE_BIN  - Path to firmware.bin (default: firmware.bin in this dir)
::   VERIFY        - Set to 1 to enable verification (default: 1)
:: ============================================================================

if not defined OPENOCD_ROOT set "OPENOCD_ROOT=C:\Users\rinry\Tool\OpenOCD-Nuvoton"

set "OPENOCD_BIN=%OPENOCD_ROOT%\OpenOCD\bin\openocd.exe"
set "OPENOCD_SCR=%OPENOCD_ROOT%\OpenOCD\scripts"
set "SCRIPT_DIR=%~dp0"
if not defined FIRMWARE_BIN set "FIRMWARE_BIN=%SCRIPT_DIR%firmware.bin"

:: Default verify=1 unless explicitly disabled
if not defined VERIFY set "VERIFY=1"

:: --------------------------------------------------------------------------
:: 1. Validate OpenOCD tool exists
:: --------------------------------------------------------------------------
if not exist "%OPENOCD_BIN%" (
    echo [ERROR] OpenOCD executable not found: %OPENOCD_BIN%
    echo Please set OPENOCD_ROOT environment variable.
    exit /b 1
)

:: --------------------------------------------------------------------------
:: 2. Validate firmware binary exists
:: --------------------------------------------------------------------------
if not exist "%FIRMWARE_BIN%" (
    echo [ERROR] Firmware binary not found: %FIRMWARE_BIN%
    echo Please build the firmware first with: mingw32-make all
    exit /b 2
)

:: --------------------------------------------------------------------------
:: 3. Validate OpenOCD scripts directory
:: --------------------------------------------------------------------------
if not exist "%OPENOCD_SCR%" (
    echo [ERROR] OpenOCD scripts directory not found: %OPENOCD_SCR%
    exit /b 3
)

:: --------------------------------------------------------------------------
:: 4. Execute OpenOCD with optional verify
:: --------------------------------------------------------------------------
echo [INFO] Flashing: %FIRMWARE_BIN%
echo [INFO] Target: M480 (M487)
if "%VERIFY%"=="1" (
    echo [INFO] Verification: enabled
) else (
    echo [INFO] Verification: disabled
)
echo.

if "%VERIFY%"=="1" (
    "%OPENOCD_BIN%" -s "%OPENOCD_SCR%" -f interface/nulink.cfg -f target/numicroM4.cfg -c "init" -c "reset halt" -c "flash write_image erase %FIRMWARE_BIN% 0" -c "verify_image %FIRMWARE_BIN% 0" -c "reset run" -c "shutdown"
) else (
    "%OPENOCD_BIN%" -s "%OPENOCD_SCR%" -f interface/nulink.cfg -f target/numicroM4.cfg -c "init" -c "reset halt" -c "flash write_image erase %FIRMWARE_BIN% 0" -c "reset run" -c "shutdown"
)
set "EXIT_CODE=%errorlevel%"

if %EXIT_CODE% neq 0 (
    echo.
    echo [ERROR] Flash failed with exit code: %EXIT_CODE%
    exit /b %EXIT_CODE%
)

echo.
echo [SUCCESS] Flash completed successfully.
if "%VERIFY%"=="1" echo [SUCCESS] Verification passed.
exit /b 0
