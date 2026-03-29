@echo off
REM
REM build.cmd - Build M487Filter user-mode applications
REM
REM Usage:
REM   build.cmd           Build all apps (hidlog, M487FilterWmiApp)
REM   build.cmd hidlog    Build hidlog only
REM   build.cmd wmiapp    Build M487FilterWmiApp only
REM   build.cmd clean     Clean build outputs
REM
REM Requirements:
REM   - Visual Studio 2019 or 2022 with C++ tools installed
REM   - Or WDK installed (provides cl.exe and Windows SDK)
REM

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "BUILD_DIR=%SCRIPT_DIR%build"
set "SRC_DIR=%SCRIPT_DIR%"

REM Detect Visual Studio
if defined VSINSTALLDIR goto vs_found
if defined VCToolsInstallDir goto vs_found

REM Try Visual Studio 2022
where vcvars64.bat >nul 2>&1
if !errorlevel!==0 (
    call vcvars64.bat
    goto vs_found
)

REM Try Developer Command Prompt
if exist "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat" (
    call "%VSINSTALLDIR%\VC\Auxiliary\Build\vcvars64.bat"
    goto vs_found
)

echo [ERROR] Visual Studio not found. Please run from a Visual Studio Developer Command Prompt.
exit /b 1

:vs_found
echo [INFO] Using Visual Studio at: %VSINSTALLDIR%

REM Parse arguments
set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=ALL"

if "%TARGET%"=="clean" (
    echo [INFO] Cleaning build outputs...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
    echo [INFO] Done.
    exit /b 0
)

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Compiler flags
set "CFLAGS=/EHsc /W4 /nologo /utf-8 /D_UNICODE /DUNICODE"
set "LFLAGS=/nologo /machine:x64"

if "%TARGET%"=="ALL" goto build_all
if "%TARGET%"=="hidlog" goto build_hidlog
if "%TARGET%"=="wmiapp" goto build_wmiapp

echo [ERROR] Unknown target: %TARGET%
echo Usage: build.cmd [hidlog^|wmiapp^|clean]
exit /b 1

:build_all
echo [INFO] Building all apps...
call :build_hidlog
call :build_wmiapp
echo [INFO] Build complete.
exit /b 0

:build_hidlog
echo [INFO] Building hidlog.exe...
set "OUT_DIR=%BUILD_DIR%\hidlog"
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

cl %CFLAGS% ^
    "%SRC_DIR%hidlog.c" ^
    /Fe"%OUT_DIR%\hidlog.exe" ^
    /link advapi32.lib ^
    %LFLAGS%
if errorlevel 1 (
    echo [ERROR] hidlog build failed.
    exit /b 1
)
echo [OK] hidlog.exe built.
exit /b 0

:build_wmiapp
echo [INFO] Building M487FilterWmiApp.exe...
set "OUT_DIR=%BUILD_DIR%\M487FilterWmiApp"
if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

cl %CFLAGS% ^
    "%SRC_DIR%M487FilterWmiApp.c" ^
    /Fe"%OUT_DIR%\M487FilterWmiApp.exe" ^
    /link ole32.lib oleaut32.lib uuid.lib ^
    %LFLAGS%
if errorlevel 1 (
    echo [ERROR] M487FilterWmiApp build failed.
    exit /b 1
)
echo [OK] M487FilterWmiApp.exe built.
exit /b 0
