# PLAN.md — T035

## 執行計畫

參考工具，無需主動執行。

1. 安裝 Wireshark + USBPcap
2. 選擇 USBPcap 介面
3. Filter: `usb.device_address == <M487_ADDR>`
4. 分析 USB HID 封包、MSC BOT 傳輸
