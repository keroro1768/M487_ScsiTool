@echo off
setlocal enabledelayedexpansion

::============================================================
:: M487Filter Driver Installer
:: 
:: Usage:
::   install.bat                    - Interactive install
::   install.bat test-sign          - Install with test signing enabled
::   install.bat ev-sign [pfx] [pwd]- Install with EV certificate
::
:: Requirements:
::   - M487Filter.sys must exist in the same directory
::   - M487Filter.cat must exist (for production install)
::   - Admin rights required
::============================================================

set "SCRIPT_DIR=%~dp0"
set "DRIVER_NAME=M487Filter"
set "INF_FILE=%SCRIPT_DIR%M487Filter.inf"
set "SYS_FILE=%SCRIPT_DIR%M487Filter.sys"
set "CAT_FILE=%SCRIPT_DIR%M487Filter.cat"

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
echo   M487Filter Driver Installer
echo ============================================================
echo.

::------------------------------------------------------------
:: Parse arguments
::------------------------------------------------------------
set "SIGN_MODE=%1"
set "EV_PFX=%2"
set "EV_PWD=%3"

if "%SIGN_MODE%"=="" set "SIGN_MODE=auto"

::------------------------------------------------------------
:: Step 1: Verify prerequisites
::------------------------------------------------------------
echo [Step 1/5] Checking prerequisites...

if not exist "%SYS_FILE%" (
    echo [ERROR] M487Filter.sys not found in:
    echo   %SYS_FILE%
    echo.
    echo Please build the driver first:
    echo   cd driver
    echo   build -g -km
    pause
    exit /b 1
)

:: Check if .inf exists
if not exist "%INF_FILE%" (
    echo [ERROR] M487Filter.inf not found in:
    echo   %INF_FILE%
    pause
    exit /b 1
)

echo   [OK] Driver files found

::------------------------------------------------------------
:: Step 2: Device detection
::------------------------------------------------------------
echo.
echo [Step 2/5] Detecting M487 devices...

:: Try to find M487 device via devcon or powershell
set "DEVICE_FOUND="
set "DEVICE_HWID=HID\Vid_0416&Pid_501E"

:: Method 1: Try devcon if available
where devcon >nul 2>&1
if %errorlevel% equ 0 (
    devcon find "HID\Vid_0416&Pid_501E" >nul 2>&1
    if %errorlevel% equ 0 (
        set "DEVICE_FOUND=1"
        echo   [OK] M487 device found (VID_0416 PID_501E)
    )
)

:: Method 2: Try PowerShell WMI
if not defined DEVICE_FOUND (
    powershell -Command "Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like '*VID_0416*PID_501E*' }" | findstr /i "VID_0416" >nul
    if %errorlevel% equ 0 (
        set "DEVICE_FOUND=1"
        echo   [OK] M487 device found (via WMI)
    )
)

if not defined DEVICE_FOUND (
    echo   [WARN] No M487 device detected.
    echo          The driver will be prepared for manual installation.
    echo.
)

::------------------------------------------------------------
:: Step 3: Driver signing
::------------------------------------------------------------
echo.
echo [Step 3/5] Driver signing...

if /i "%SIGN_MODE%"=="test-sign" (
    echo   [MODE] Test Signing - Enabling test signing mode...
    call :enable-test-signing
    if !errorlevel! neq 0 (
        echo   [FAIL] Failed to enable test signing
        pause
        exit /b 1
    )
    
    :: Self-sign the driver
    call :test-sign-driver
    if !errorlevel! neq 0 (
        echo   [FAIL] Failed to sign driver
        pause
        exit /b 1
    )
    
) else if /i "%SIGN_MODE%"=="ev-sign" (
    if not defined EV_PFX (
        echo [ERROR] EV certificate PFX file path required
        echo Usage: install.bat ev-sign [pfx_path] [password]
        pause
        exit /b 1
    )
    call :ev-sign-driver "%EV_PFX%" "%EV_PWD%"
    if !errorlevel! neq 0 (
        echo   [FAIL] EV signing failed
        pause
        exit /b 1
    )
    
) else if /i "%SIGN_MODE%"=="auto" (
    :: Auto-detect: Check if already signed or use test signing
    if exist "%CAT_FILE%" (
        echo   [OK] Catalog file exists, checking signature...
        powershell -Command "Get-AuthenticodeSignature '%CAT_FILE%' | Select-Object -ExpandProperty Status" | findstr /i "Valid" >nul
        if %errorlevel% equ 0 (
            echo   [OK] Driver already has valid signature
        ) else (
            echo   [INFO] Catalog exists but signature invalid/incomplete
            echo          Will attempt test signing...
            call :enable-test-signing >nul 2>&1
            call :test-sign-driver
        )
    ) else (
        echo   [INFO] No catalog file - test signing required
        call :enable-test-signing >nul 2>&1
        call :test-sign-driver
    )
)

::------------------------------------------------------------
:: Step 4: Add to Driver Store
::------------------------------------------------------------
echo.
echo [Step 4/5] Adding driver to Windows Driver Store...

:: Copy to a staging area first
if not exist "%TEMP%\M487Filter_install" mkdir "%TEMP%\M487Filter_install"
copy /Y "%INF_FILE%" "%TEMP%\M487Filter_install\" >nul
copy /Y "%SYS_FILE%" "%TEMP%\M487Filter_install\" >nul
if exist "%CAT_FILE%" copy /Y "%CAT_FILE%" "%TEMP%\M487Filter_install\" >nul

:: Use pnputil to add to driver store
pnputil /add-driver "%TEMP%\M487Filter_install\M487Filter.inf" /install >nul 2>&1
set "PNP_RESULT=%errorlevel%"

if %PNP_RESULT% equ 0 (
    echo   [OK] Driver added to system
) else (
    echo   [WARN] pnputil returned %PNP_RESULT% - may need manual install
)

::------------------------------------------------------------
:: Step 5: Attempt device installation
::------------------------------------------------------------
echo.
echo [Step 5/5] Installing driver on device...

if defined DEVICE_FOUND (
    echo   Attempting to install on detected device...
    
    :: Method 1: Update driver on existing device
    where devcon >nul 2>&1
    if !errorlevel! equ 0 (
        devcon update "%INF_FILE%" "HID\Vid_0416&Pid_501E" >nul 2>&1
        if !errorlevel! equ 0 (
            echo   [OK] Driver updated via devcon
        ) else (
            echo   [INFO] devcon update returned non-zero
        )
    )
    
    :: Method 2: Use PowerShell UpdateDriver
    powershell -Command "Update-Driver -HardwareId 'HID\Vid_0416&Pid_501E' -SearchDriverStore -Force" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [OK] Driver updated via PowerShell
    )
) else (
    echo   [INFO] No device found. Device installation will occur on next plug-in.
    echo.
    echo   To manually install later:
    echo   1. Connect the M487 device
    echo   2. Open Device Manager
    echo   3. Find the M487 device under HID devices
    echo   4. Right-click ^> Update driver ^> Browse my computer
    echo   5. Point to: %SCRIPT_DIR%
)

::------------------------------------------------------------
:: Cleanup and summary
::------------------------------------------------------------
echo.
echo ============================================================
echo   Installation Complete
echo ============================================================
echo.
echo   Driver:  %DRIVER_NAME%.sys
echo   Status:  Installed in Driver Store
echo   Mode:    %SIGN_MODE%
echo.
echo   NOTE: A system restart may be required if this is
echo         the first time installing a test-signed driver.
echo.
echo   To verify installation:
echo   devcon status "HID\Vid_0416&Pid_501E"
echo   or
echo   Get-WindowsDriver -Online ^| where { $_.Driver -like '*M487*' }
echo.
pause
exit /b 0

::============================================================
:: Subroutines
::============================================================

:enable-test-signing
    :: Check if test signing already enabled
    bcdedit /enum testsigning | findstr /i "Yes" >nul
    if %errorlevel% equ 0 (
        echo     Test signing already enabled
        exit /b 0
    )
    
    echo     Enabling test signing (requires reboot)...
    bcdedit /set testsigning on >nul 2>&1
    if %errorlevel% neq 0 (
        echo     [WARN] Failed to enable test signing via bcdedit
        echo            Try running as admin or check Boot Security settings
        exit /b 1
    )
    echo     [OK] Test signing enabled - reboot required
    exit /b 0

:test-sign-driver
    :: Verify signtool available
    where signtool >nul 2>&1
    if %errorlevel% neq 0 (
        :: Try to find in SDK
        for /d %%D in ("%ProgramFiles(x86)%\Windows Kits\10\bin\10.*") do (
            if exist "%%D\x64\signtool.exe" (
                set "SIGNTOOL=%%D\x64\signtool.exe"
            )
        )
        if not defined SIGNTOOL (
            echo     [WARN] signtool not found - skipping signature
            exit /b 0
        )
    ) else (
        set "SIGNTOOL=signtool"
    )
    
    :: Generate self-signed certificate if not exists
    set "TEST_CERT=%TEMP%\M487Filter_TestCert.pfx"
    set "TEST_CERT_PWD=M487TestOnly"
    
    if not exist "%TEST_CERT%" (
        echo     Creating self-signed test certificate...
        powershell -Command "New-SelfSignedCertificate -Type Custom -Subject 'CN=M487 Test' -KeyUsage DigitalSignature -FriendlyName 'M487Filter Test Cert' -CertStoreLocation 'Cert:\CurrentUser\My' -TextExtension @('2.5.29.37={text}1.3.6.1.5.5.7.3.3','2.5.29.19={text}')" >nul 2>&1
        
        :: Export to PFX
        powershell -Command "$cert = Get-ChildItem -Path 'Cert:\CurrentUser\My' | Where-Object { $_.Subject -like '*M487 Test*' } | Select-Object -First 1; if ($cert) { $cert | Export-PfxCertificate -FilePath '%TEST_CERT%' -Password (ConvertTo-SecureString -String '%TEST_CERT_PWD%' -Force -AsPlainText) }" >nul 2>&1
        
        if not exist "%TEST_CERT%" (
            echo     [WARN] Could not create test certificate
            exit /b 1
        )
    )
    
    :: Sign the driver using inf2cat first
    echo     Running Inf2Cat to generate catalog...
    
    :: Find INF2CAT
    set "INF2CAT="
    for /d %%D in ("%ProgramFiles(x86)%\Windows Kits\10\bin\10.*") do (
        if exist "%%D\x64\inf2cat.exe" (
            set "INF2CAT=%%D\x64\inf2cat.exe"
        )
    )
    
    if defined INF2CAT (
        "%INF2CAT%" /v2 /driver:"%SCRIPT_DIR%" /os:10_x64 >nul 2>&1
        if exist "%SCRIPT_DIR%M487Filter.cat" (
            echo     Catalog generated: M487Filter.cat
        )
    ) else (
        echo     [WARN] inf2cat not found - using existing catalog or skipping
    )
    
    :: Sign the catalog if exists
    if exist "%SCRIPT_DIR%M487Filter.cat" (
        echo     Signing catalog...
        "%SIGNTOOL%" sign /fd SHA256 /f "%TEST_CERT%" /p "%TEST_CERT_PWD%" "%SCRIPT_DIR%M487Filter.cat" >nul 2>&1
        if %errorlevel% equ 0 (
            echo     [OK] Driver catalog signed
        ) else (
            echo     [WARN] Signing returned non-zero
        )
    ) else (
        echo     [WARN] No catalog file to sign
    )
    
    exit /b 0

:ev-sign-driver
    set "EV_PFX=%~1"
    set "EV_PWD=%~2"
    
    if not exist "%EV_PFX%" (
        echo     [ERROR] Certificate file not found: %EV_PFX%
        exit /b 1
    )
    
    where signtool >nul 2>&1
    if %errorlevel% neq 0 (
        echo     [ERROR] signtool not found in PATH
        exit /b 1
    )
    
    echo     Signing with EV certificate...
    
    if exist "%SCRIPT_DIR%M487Filter.cat" (
        "%SIGNTOOL%" sign /fd SHA256 /a /f "%EV_PFX%" /p "%EV_PWD%" "%SCRIPT_DIR%M487Filter.cat"
        if %errorlevel% equ 0 (
            echo     [OK] Catalog signed with EV certificate
        ) else (
            echo     [ERROR] Catalog signing failed
            exit /b 1
        )
    ) else (
        echo     [ERROR] M487Filter.cat not found. Run inf2cat first.
        exit /b 1
    )
    
    exit /b 0
