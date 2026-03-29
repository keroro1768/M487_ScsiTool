# Task.md — T003

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T003 |
| 標題 | OpenOCD + Nu-Link 燒錄流程建立 |
| 狀態 | done |
| 優先序 | - |
| 指派 | Giroro |
| 依賴 | T002 |
| 截止 | - |

## 目標

建立 M487 韌體的 OpenOCD + Nu-Link 燒錄流程

## 需求

- [x] Nuvoton OpenOCD 安裝驗證
- [x] Nu-Link USB 驅動設定
- [x] 燒錄指令驗證
- [x] flash.bat 脚本建立

## 進度

**工具：**
- OpenOCD: `C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe`
- 燒錄器: Nu-Link（VID=0x0416, PID=0x511c）

**燒錄速度：**
- VENDOR_LBK (43KB): ~13 KiB/s
- Composite (49KB): ~6.7 KiB/s

## 備註

⚠️ OpenOCD 驅動目前有 LIBUSB_ERROR_ACCESS 問題，需 Zadig 修復
