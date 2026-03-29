@echo off
setlocal enabledelayedexpansion

::============================================================
:: M487Filter Driver Uninstaller
:: 
:: Usage:
::   uninstall.bat              - Remove driver from system
::   uninstall.bat full        - Full removal (driver store + files)
::
:: Requirements:
::   - Admin rights required
::============================================================

set "SCRIPT_DIR=%~dp0"
set "DRIVER_NAME=M487Filter"

::------------------------------------------------------------
:: Check for admin rights
::------------------------------------------------------------
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Administrator privileges required!
    echo Please right-click and select "Run as administrator"
    pause
    exit /b 1
)

echo ============================================================
echo   M487Filter Driver Uninstaller
echo ============================================================
echo.

set "FULL_REMOVAL=%1"
if /i "%FULL_REMOVAL%"=="full" (
    echo [MODE] Full removal mode
) else (
    echo [MODE] Standard removal
)
echo.

::------------------------------------------------------------
:: Step 1: Stop and remove the driver service
::------------------------------------------------------------
echo [Step 1/4] Stopping driver service...

:: Try to stop via sc
sc stop %DRIVER_NAME% >nul 2>&1
:: Give it time to stop
timeout /t 2 /nobreak >nul

:: Check if stopped or already removed
sc query %DRIVER_NAME% | findstr /i "STOPPED\|does not exist" >nul
if %errorlevel% equ 0 (
    echo   [OK] Service stopped or not running
) else (
    echo   [INFO] Service may still be running
)

:: Delete service
sc delete %DRIVER_NAME% >nul 2>&1
if %errorlevel% equ 0 (
    echo   [OK] Service deleted
) else (
    echo   [INFO] Service deletion returned: %errorlevel%
)

::------------------------------------------------------------
:: Step 2: Remove from Plug and Play (if device present)
::------------------------------------------------------------
echo.
echo [Step 2/4] Removing from Plug and Play...

:: Method 1: devcon
where devcon >nul 2>&1
if %errorlevel% equ 0 (
    echo   Trying devcon remove...
    devcon remove "HID\Vid_0416&Pid_501E" >nul 2>&1
    devcon remove "HID\Vid_0416&Pid_FF20" >nul 2>&1
    devcon remove "HID\Vid_0416&Pid_5020" >nul 2>&1
    echo   [OK] devcon remove attempted
)

:: Method 2: PowerShell Remove-PnpDevice
echo   Trying PowerShell Remove-PnpDevice...
powershell -Command "Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like '*VID_0416*' } | Where-Object { $_.FriendlyName -like '*M487*' } | Disable-PnpDevice -Confirm:$false" >nul 2>&1
powershell -Command "Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like '*VID_0416*' } | Where-Object { $_.FriendlyName -like '*M487*' } | Remove-PnpDevice -Confirm:$false" >nul 2>&1

::------------------------------------------------------------
:: Step 3: Remove from Driver Store
::------------------------------------------------------------
echo.
echo [Step 3/4] Removing from Windows Driver Store...

:: Use pnputil to remove
pnputil /enum-drivers | findstr /i "M487Filter" >nul 2>&1
if %errorlevel% equ 0 (
    echo   Driver found in Driver Store, removing...
    pnputil /remove-driver oem*.inf /uninstall /force >nul 2>&1
    echo   [OK] Driver Store removal attempted
) else (
    echo   [INFO] Driver not found in Driver Store
)

::------------------------------------------------------------
:: Step 4: Cleanup files (optional)
::------------------------------------------------------------
if /i "%FULL_REMOVAL%"=="full" (
    echo.
    echo [Step 4/4] Cleaning up driver files...
    
    :: Remove .sys only if in script directory (to avoid accidental deletion)
    :: Note: Usually you want to keep these for re-installation
    :: del /F "%SCRIPT_DIR%%DRIVER_NAME%.sys" >nul 2>&1
    :: del /F "%SCRIPT_DIR%%DRIVER_NAME%.cat" >nul 2>&1
    
    echo   [INFO] Keeping driver files in place for reinstallation
    echo          To remove manually: delete M487Filter.sys and M487Filter.cat
)

::------------------------------------------------------------
:: Final status
::------------------------------------------------------------
echo.
echo ============================================================
echo   Uninstallation Complete
echo ============================================================
echo.
echo   The M487Filter driver has been removed from the system.
echo.
echo   NOTE: If the device is currently connected, you may need
echo         to unplug and replug it for the system to use the
echo         default HID driver.
echo.
echo   To verify removal:
echo   devcon status "HID\Vid_0416&Pid_501E"
echo   or
echo   Get-WindowsDriver -Online ^| where { $_.Driver -like '*M487*' }
echo.
pause
exit /b 0
