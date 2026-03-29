# Task.md — T006

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T006 |
| 標題 | Windows C++ Win32 HID Tool |
| 狀態 | open |
| 優先序 | P2 |
| 指派 | Tamama |
| 依賴 | T004 / T008 完成後 |
| 截止 | - |

## 目標

Windows CLI 工具，與 M487 USB HID 溝通

## 需求

- [ ] 研究 Windows HID API（HidD_*, SetupDi*）
- [ ] 設計乾淨的 C++ class 介面
- [ ] 實作 HID I2C 命令傳送/接收
- [ ] 整合測試

## 進度

規格文件已完成：`tool/hidtool/SPEC.md`
