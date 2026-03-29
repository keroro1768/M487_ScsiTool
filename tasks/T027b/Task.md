# Task.md — T027b

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T027b |
| 標題 | Firefly 範本研究 |
| 狀態 | open |
| 優先序 | P1 |
| 指派 | Tamama |
| 依賴 | 無（範本可從網路下載）|
| 截止 | - |

## 目標

分析 Windows-driver-samples/hid/firefly，掌握 HID Filter Driver 實作模式

## 需求

- [ ] Clone Windows-driver-samples（或直接下載 ZIP）
  - 下載：https://github.com/microsoft/Windows-driver-samples/archive/refs/heads/main.zip
- [ ] 分析 firefly HID 範例程式碼
  - 路徑：`Windows-driver-samples/hid/firefly/`
- [ ] 提取關鍵架構：Upper Filter attach/detach, IOCTL 攔截
