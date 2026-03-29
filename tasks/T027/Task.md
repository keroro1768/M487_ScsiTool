# Task.md — T027

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T027 |
| 標題 | USB HID Filter Driver（Windows Kernel Debug Tool）|
| 狀態 | open |
| 優先序 | P1 |
| 指派 | Tamama |
| 依賴 | T027a WDK 安裝完成 |
| 截止 | - |

## 目標

建立 Windows Kernel-mode Filter Driver，附掛在 HID Class Driver 之上，攔截並紀錄所有 USB HID 溝通封包

## 需求

- [ ] T027a: WDK + VS2022 環境建立
- [ ] T027b: Firefly 範本研究
- [ ] T027c: Filter Driver 骨架實作
- [ ] T027d: IOCTL 攔截實作
- [ ] T027e: Ring Buffer + ETW
- [ ] T027f: WMI Interface
- [ ] T027g: hidlog.exe CLI 工具
- [ ] T027h: 安裝程式/簽章

## 進度

T027a（WDK 安裝）待完成
