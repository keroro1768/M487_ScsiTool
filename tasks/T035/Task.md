# Task.md — T035

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T035 |
| 標題 | USBPcap + Wireshark 整合 |
| 狀態 | open |
| 優先序 | P3 |
| 指派 | 無（參考工具）|
| 依賴 | 無 |
| 截止 | - |

## 目標

使用 USBPcap 擷取 USB 流量，用於進階協定分析

## 需求

- [ ] 安裝 Wireshark + USBPcap
- [ ] Filter: usb.device_address == <M487_ADDR>
- [ ] 分析 USB HID 封包、MSC BOT 傳輸

## 備註

只能離線分析，適合開發階段，不適合客戶現場
