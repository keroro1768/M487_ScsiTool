# Task.md — T037

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T037 |
| 標題 | 微軟 USB Driver Samples 研究 |
| 狀態 | open |
| 優先序 | P1 |
| 指派 | Tamama |
| 依賴 | 無 |
| 截止 | - |

## 目標

研究微軟 Windows-driver-samples 中的 USB 範例，掌握 USB Filter Driver / USB Device Driver 實作模式

## 知識庫位置

`D:\AiWorkSpace\M487_ScsiTool\doc\USB_Driver_Samples\`

## 需求

- [ ] 分析 USB 相關範例（`usb/` 目錄下）
- [ ] 分析 USB HID 範例
- [ ] 分析 USB Filter Driver 範例
- [ ] 提取關鍵架構：USB 描述符、URB 處理、IRP 堆疊
- [ ] 建立研究報告

## Windows-driver-samples 路徑

```
Windows-driver-samples/usb/
├── usbsamp/              ← USB 範例
├── usbview/              ← USB 檢視工具
├── kmdf_fx2/            ← USB FX2 開發板範例
├── hid/                  ← HID 範例（含 firefly）
└── filter/               ← USB Filter 範例
```
