<#
NULink WinUSB Driver Setup Script
=================================
自動使用 Zadig 將 Nu-Link Interface 0 的驅動程式更換為 WinUSB

使用方法（需系統管理員權限）：
  右鍵點擊此腳本 → "以系統管理員身份執行"
  或：powershell -ExecutionPolicy Bypass -File C:\Users\rinry\Tool\NULink_Setup.ps1

檔案需求：
  - Zadig:       C:\Users\rinry\Tool\zadig-2.9.exe
  - 預設檔:       C:\Users\rinry\Tool\NULink.preset
#>

param(
    [string]$ZadigPath    = "C:\Users\rinry\Tool\zadig-2.9.exe",
    [string]$PresetPath    = "C:\Users\rinry\Tool\NULink.preset",
    [switch]$ManualOnly     # 加這個參數只顯示手動說明，不啟動 Zadig
)

$ErrorActionPreference = "Continue"

function Get-NuLinkDevice {
    $dev = Get-PnpDevice -InstanceId "HID\VID_0416&PID_511C&MI_00*" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($dev) {
        $infPath = Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName "DEVPKEY_Device_DriverInfPath" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Data
        $provider = Get-PnpDeviceProperty -InstanceId $dev.InstanceId -KeyName "DEVPKEY_Device_DriverProvider" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Data
        [PSCustomObject]@{
            InstanceId = $dev.InstanceId
            Name       = $dev.FriendlyName
            Status     = $dev.Status
            DriverInf  = $infPath
            Provider   = $provider
        }
    }
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Nu-Link WinUSB 驅動程式設定" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 預先檢查
Write-Host "[1] 檢查 Nu-Link 裝置狀態..." -ForegroundColor Yellow
$devInfo = Get-NuLinkDevice
if ($devInfo) {
    Write-Host "    裝置: $($devInfo.Name)" -ForegroundColor White
    Write-Host "    狀態: $($devInfo.Status)" -ForegroundColor White
    Write-Host "    驅動 INF: $($devInfo.DriverInf)" -ForegroundColor White
    Write-Host "    驅動供應商: $($devInfo.Provider)" -ForegroundColor White
    
    if ($devInfo.DriverInf -like "*winusb*") {
        Write-Host ""
        Write-Host "    [OK] WinUSB 驅動已安裝！跳過安裝步驟。" -ForegroundColor Green
        Write-Host ""
        Write-Host "    驗證 OpenOCD/pyOCD 連接：" -ForegroundColor Cyan
        Write-Host '      python -m pyocd list' -ForegroundColor Gray
        Write-Host '      openocd -f interface/cmsis-dap.cfg -f target/nuvoton_nuc4xx.cfg' -ForegroundColor Gray
        exit 0
    } elseif ($devInfo.DriverInf -like "*input.inf*" -or $devInfo.DriverInf -like "*hid*") {
        Write-Host "    [需更換] 目前使用 HID 驅動，需要更換為 WinUSB。" -ForegroundColor Yellow
    } else {
        Write-Host "    [未知] 驅動類型: $($devInfo.DriverInf)" -ForegroundColor Yellow
    }
} else {
    Write-Host "    [警告] 找不到 Nu-Link HID 介面 0 裝置。" -ForegroundColor Red
    Write-Host "    請確認 Nu-Link 已透過 USB 連接到電腦。" -ForegroundColor Yellow
}

# 檔案檢查
Write-Host ""
Write-Host "[2] 檢查所需檔案..." -ForegroundColor Yellow
if (Test-Path $ZadigPath) {
    $zVersion = (Get-Item $ZadigPath).VersionInfo.FileVersion
    Write-Host "    Zadig: $ZadigPath (v$zVersion)" -ForegroundColor Green
} else {
    Write-Host "    [錯誤] 找不到 Zadig: $ZadigPath" -ForegroundColor Red
    exit 1
}

if (Test-Path $PresetPath) {
    Write-Host "    預設檔: $PresetPath" -ForegroundColor Green
} else {
    Write-Host "    [錯誤] 找不到預設檔: $PresetPath" -ForegroundColor Red
    exit 1
}

if ($ManualOnly) {
    Write-Host ""
    Write-Host "[X] ManualOnly 模式：略過 Zadig 啟動。" -ForegroundColor Cyan
    Write-Host ""
    goto MANUAL_SECTION
}

# 啟動 Zadig
Write-Host ""
Write-Host "[3] 啟動 Zadig（將彈出 UAC 提示）..." -ForegroundColor Yellow
try {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ZadigPath
    $psi.UseShellExecute = $true
    $psi.Verb = "runas"
    $psi.WorkingDirectory = Split-Path $ZadigPath
    $null = [System.Diagnostics.Process]::Start($psi)
    Write-Host "    Zadig 已啟動。" -ForegroundColor Green
    Write-Host "    請在 Zadig 視窗中完成以下操作..." -ForegroundColor Cyan
} catch {
    Write-Host "    [錯誤] 無法啟動 Zadig: $_" -ForegroundColor Red
}

MANUAL_SECTION:
Write-Host ""
Write-Host "========================================" -ForegroundColor Yellow
Write-Host " 手動操作說明（Zadig GUI）" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Yellow
Write-Host ""
Write-Host "步驟 1：在 Zadig 中載入預設檔" -ForegroundColor White
Write-Host "  按下 Device 功能表 → Load Preset Device" -ForegroundColor Gray
Write-Host "  選擇檔案: $PresetPath" -ForegroundColor Gray
Write-Host ""
Write-Host "步驟 2：確認裝置選擇正確" -ForegroundColor White
Write-Host "  - 下拉選單中應該看到 'Nuvoton Nu-Link CMSIS-DAP (Interface 0)'" -ForegroundColor Gray
Write-Host "  - USB ID: 0416:511C [MI:00]" -ForegroundColor Gray
Write-Host "  - 如果看不到，檢查 Options → List All Devices" -ForegroundColor Gray
Write-Host ""
Write-Host "步驟 3：選擇 WinUSB 驅動" -ForegroundColor White
Write-Host "  - 驅動程式下拉選單 → 選擇 WinUSB" -ForegroundColor Gray
Write-Host ""
Write-Host "步驟 4：點擊 Replace Driver（或 Install Driver）" -ForegroundColor White
Write-Host "  - 等候安裝完成（約 10-30 秒）" -ForegroundColor Gray
Write-Host "  - 成功後狀態列會顯示綠色 WinUSB" -ForegroundColor Gray
Write-Host ""
Write-Host "步驟 5：驗證" -ForegroundColor White
Write-Host "  - 執行: python -m pyocd list" -ForegroundColor Gray
Write-Host "  - 或: openocd -f interface/cmsis-dap.cfg -f target/nuvoton_nuc4xx.cfg" -ForegroundColor Gray
Write-Host ""

# 等候後再次檢查
if (-not $ManualOnly) {
    Write-Host "[4] 等候 5 秒後檢查驅動狀態..." -ForegroundColor Yellow
    Start-Sleep -Seconds 5
    
    $devInfo2 = Get-NuLinkDevice
    if ($devInfo2) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host " 驅動狀態檢查" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "  驅動 INF: $($devInfo2.DriverInf)" -ForegroundColor White
        
        if ($devInfo2.DriverInf -like "*winusb*") {
            Write-Host "  [成功！] WinUSB 驅動已安裝。" -ForegroundColor Green
        } else {
            Write-Host "  [未變更] 驅動仍是: $($devInfo2.DriverInf)" -ForegroundColor Yellow
            Write-Host "  請確認在 Zadig 中點擊了 Replace Driver 按鈕。" -ForegroundColor Yellow
        }
    }
}
