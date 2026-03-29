@echo off
REM hidtool launcher - Unified HID + MSC Debug Channel CLI
REM Requires Python 3.8+ with hidapi package: pip install hidapi

setlocal
set PYTHON=
REM Try to find Python
if exist "C:\Users\rinry\AppData\Local\Programs\Python\Python314\python.exe" (
    set PYTHON=C:\Users\rinry\AppData\Local\Programs\Python\Python314\python.exe
) else (
    where python >nul 2>&1
    if not errorlevel 1 set PYTHON=python
    if not defined PYTHON (
        where python3 >nul 2>&1
        if not errorlevel 1 set PYTHON=python3
    )
)
if not defined PYTHON (
    echo [ERROR] Python not found. Install Python 3.8+ and run: pip install hidapi
    exit /b 1
)

set SCRIPT_DIR=%~dp0
"%PYTHON%" "%SCRIPT_DIR%hidtool.py" %*
