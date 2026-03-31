# Task.md — T001

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T001 |
| 標題 | USB 複合裝置（MSC + HID I2C Bridge）|
| 狀態 | ⏸️ 暫停（等硬體 power-cycle）|
| 優先序 | P0 |
| 指派 | Dororo |
| 依賴 | 硬體（M487 USB Device 纜線 + power-cycle）|
| 截止 | - |

## 目標

在 M487 上實作 USB 複合裝置，包含 HID I2C Bridge + USB Mass Storage，VID=0x04F3 PID=0x0732

## 需求

- [x] HID I2C Bridge（Interrupt EP1/EP2）
- [x] USB Mass Storage（Bulk EP3/EP4，30KB RAM Disk）
- [x] I2C_Read() 功能實作 ✅（i2c_control.c 已完整實作，含 NACK retry）
- [ ] 實體測試（等 power-cycle 確認 USB 枚舉）
- [ ] Windows C++ Win32 Tool 整合測試

## 進度

**韌體進度：**
- Branch: `firmware/composite-rewrite`
- 韌體架構設計 ✅
- GCC 編譯環境建立 ✅
- 韌體編譯成功（61.2KB）✅
- 燒錄驗證 ✅（OpenOCD + Nu-Link, 2026-03-30）
- VSCode F5 Debug 驗證 ✅（2026-03-30）
- I2C_Read() 功能完整實作 ✅（確認於 2026-03-31）
- USB HID Layer 完整整合（BSP HSUSBD）✅
- HID_CmdI2CRead() 完整實作 ✅

**待解決（硬體依賴）：**
- USB 枚舉驗證（VID=0x04F3 PID=0x0732）← 需回辦公室 power-cycle
- HID I2C 通訊實體驗證
- MSC RAM Disk 實體驗證

## 備註

- USB VID/PID 已更新為 `0x04F3/0x0732`（正式註冊）
- 端點配置：EP1 Interrupt IN, EP2 Interrupt OUT, EP3/4 Bulk
- **OpenOCD cfg 修復（2026-03-31）：** `nulink_m487_ice.cfg` 遺失 `hla layout nulink`，已修復
- I2C_Read() 流程：START → ADDR+R → DATA... → STOP（含 NACK retry, buffer 邊界保護）
- ICE 文件已同步更新至 `doc/ICE/`
