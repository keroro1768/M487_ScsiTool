# Task.md — T001

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T001 |
| 標題 | USB 複合裝置（MSC + HID I2C Bridge）|
| 狀態 | open |
| 優先序 | P0 |
| 指派 | Dororo |
| 依賴 | - |
| 截止 | - |

## 目標

在 M487 上實作 USB 複合裝置，包含 HID I2C Bridge + USB Mass Storage，VID=0x04F3 PID=0x0732

## 需求

- [x] HID I2C Bridge（Interrupt EP1/EP2）
- [x] USB Mass Storage（Bulk EP3/EP4，30KB RAM Disk）
- [ ] I2C Read 功能實作
- [ ] 實體測試
- [ ] Windows C++ Win32 Tool 整合測試

## 進度

**韌體進度：**
- Branch: `firmware/composite-rewrite`
- 韌體架構設計 ✅
- GCC 編譯環境建立 ✅
- 韌體編譯成功（43.9KB）✅
- 燒錄驗證 ✅
- I2C Read 功能待實作
- USB HID Layer 待完整整合（BSP HSUSBD）

**待解決：**
- OpenOCD 驅動問題（LIBUSB_ERROR_ACCESS）
- 需回辦公室實測

## 備註

- USB VID/PID 已更新為 `0x04F3/0x0732`（正式註冊）
- 端點配置：EP1 Interrupt IN, EP2 Interrupt OUT, EP3/4 Bulk
- I2C Read 函式已宣告但未完整實作
