# PLAN.md — T037

## 執行計畫

### Step 1：分析 USB 範例
- `usb/usbsamp/` — 基本 USB 範例
- `usb/usbview/` — USB 檢視工具
- `usb/kmdf_fx2/` — KMDF USB 開發板範例

### Step 2：分析 USB HID 範例
- `hid/firefly/` — HID Filter Driver（已完成 T027b）
- `hid/hclient/` — HID Client 範例
- `hid/kbd/` — 鍵盤範例

### Step 3：分析 USB Filter Driver
- `usb/filter/` — USB Filter Driver

### Step 4：提取關鍵模式
- USB 描述符設定
- URB 處理流程
- IRP 堆疊管理
- PnP / Power Management

### Step 5：建立研究報告
`doc/USB_Driver_Samples/README.md`

## 預估工時

1-2 天

## 參考資源

- Windows-driver-samples: `D:\AiWorkSpace\M487_ScsiTool\Windows-driver-samples\`
- Firefly 研究（已完成 T027b）
