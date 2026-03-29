# Task.md — T027a

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T027a |
| 標題 | WDK + VS2022 環境建立 |
| 狀態 | open |
| 優先序 | P1 |
| 指派 | Tamama |
| 依賴 | - |
| 截止 | - |

## 目標

確認 WDK 安裝，建立 KMDF 專案

## 需求

- [x] 確認 Visual Studio 2022 已安裝 ✅
- [ ] 安裝 Windows Driver Kit (WDK) 10.0.26100.0+ ❌ 待安裝
- [x] 安裝 Windows 11 SDK ✅ 已具備（SDK 10.0.26100.0）
- [ ] 建立 KMDF 專案骨架

## 環境調查結果（2026-03-28）

### ✅ VS2022 Community
- 路徑：`C:\Program Files\Microsoft Visual Studio\2022\Community`
- Code CLI 版本：1.112.0

### ✅ Windows SDK 10.0.26100.0
- Headers：`C:\Program Files (x86)\Windows Kits\10\Include\10.0.26100.0\`
- KMDF 1.15 ~ 1.35 已具備
- UMDF 1.9 ~ 2.35 已具備

### ❌ WDK 10.1.26100.6584（未正確安裝）
- 登錄檔有記錄（MSI `{B9F7CBB9-E382-20AF-FF36-696A967BFF30}`）
- 但無 WDK 資料夾（`C:\Program Files (x86)\Windows Kits\10\WDK` 不存在）
- VS2022 WDK 擴充套件未安裝
- **需要重新安裝**

## WDK 下載連結

https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk

> ⚠️ WDK 需與 VS2022 版本匹配，建議安裝 WDK 10.1.26100（或與已安裝 SDK 10.0.26100.0 匹配的版本）
