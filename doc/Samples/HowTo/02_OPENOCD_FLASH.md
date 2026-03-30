# OpenOCD 燒錄流程

## ✅ 前置條件

1. Nu-Link 已連接 M487 開發板
2. Interface 1 驅動為 WinUSB（Nuvoton）
3. `C:\Users\rinry\Tool\OpenOCD-Nuvoton\` 已就緒

---

## 🔌 基本燒錄指令

### 燒錄命令列（PowerShell）

```powershell
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg `
  -f target/numicroM4.cfg `
  -c "init" `
  -c "reset halt" `
  -c "flash write_image erase <bin檔路徑> 0" `
  -c "shutdown"
```

### 燒錄（CMD）

```cmd
"C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" -f interface/nulink.cfg -f target/numicroM4.cfg -c "init" -c "reset halt" -c "flash write_image erase C:/path/to/firmware.bin 0" -c "shutdown"
```

> ⚠️ 非 admin 的 PowerShell 可能出現 `LIBUSB_ERROR_ACCESS`，需用 admin CMD 或確認 WinUSB 驅動正確

---

## 📖 完整燒錄流程

### 步驟 1：確認連線

```powershell
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg `
  -f target/numicroM4.cfg `
  -c "init" -c "targets" -c "shutdown"
```

**預期輸出：**
```
Info : IDCODE: 0x2BA01477
Info : NuMicro.cpu: hardware has 6 breakpoints, 4 watchpoints
 0* NuMicro.cpu hla_target little NuMicro.cpu halted
```

### 步驟 2：燒錄

```powershell
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg -f target/numicroM4.cfg `
  -c "init" -c "reset halt" `
  -c "flash write_image erase C:/firmware.bin 0" `
  -c "shutdown"
```

**預期輸出：**
```
wrote XXXXX bytes from file ... in X.XXXs (X.XXX KiB/s)
shutdown command invoked
```

### 步驟 3：驗證（可選）

燒錄完讀回比對 MD5：
```powershell
(Get-FileHash "C:\original.bin" -Algorithm MD5).Hash
(Get-FileHash "C:\verify.bin" -Algorithm MD5).Hash
```

---

## 📂 Flash 讀取

### 讀取全部 Flash（512KB）

```powershell
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg -f target/numicroM4.cfg `
  -c "init" -c "reset halt" `
  -c "flash read_bank 0 C:/Users/rinry/m487_flash.bin 0 0x80000" `
  -c "shutdown"
```

### 分塊讀取（顯示進度）

使用 TCL 腳本分 16 區塊讀取：
```powershell
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg -f target/numicroM4.cfg `
  -c "init" -c "reset halt" `
  -f C:/Users/rinry/Tool/read_m487_flash.tcl
```

---

## 🔧 OpenOCD 常用指令

| 指令 | 功能 |
|------|------|
| `init` | 初始化 Debug 介面 |
| `reset halt` | 重置並 halt CPU |
| `halt` | Halt CPU |
| `resume` | 繼續執行 |
| `targets` | 列出所有 targets |
| `flash read_bank 0 <file> <offset> <size>` | 讀取 Flash |
| `flash write_image erase <file> <offset>` | 燒錄 Flash（自動抹除） |
| `flash erase_address <addr> <size>` | 抹除 Flash 特定範圍 |
| `shutdown` | 關閉 OpenOCD |

---

## ⚡ 燒錄速度

| Flash 大小 | 耗時 | 速度 |
|------------|------|------|
| 4KB | ~1.3s | 3.1 KiB/s |
| 16KB (VENDOR_LBK) | ~3.4s | 5.9 KiB/s |
| 20KB (MSCSHORT) | ~3.3s | 6.1 KiB/s |
| 512KB | ~70s | 7.4 KiB/s |

> 燒錄速度瓶頸在 SWD 傳輸協定，無法透過提高時脈大幅加速

---

## ⚠️ 常見錯誤

| 錯誤 | 原因 | 解決 |
|------|------|------|
| LIBUSB_ERROR_ACCESS | USB 權限不足 | 確認 WinUSB 驅動；以 admin 執行 |
| couldn't open file | 路徑錯誤或權限不足 | 確認輸出路徑存在；用 `C:/path` 格式 |
| auto erase enabled | 正常訊息 | 無需處理 |
| Device ID: 0x00d48750 | 正常識別 M487JIDAE | — |

---

## 📝 燒錄批次檔範例

建立 `D:\AiWorkSpace\KM\M487-HowTo\flash.bat`：

```bat
@echo off
"C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" ^
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" ^
  -f interface/nulink.cfg ^
  -f target/numicroM4.cfg ^
  -c "init" ^
  -c "reset halt" ^
  -c "flash write_image erase %1 0" ^
  -c "shutdown"
pause
```

用法：`flash.bat C:\path\to\firmware.bin`
