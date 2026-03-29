# WORKLOG.md — T027a

## 2026-03-28

### 環境調查

| 項目 | 狀態 | 詳情 |
|------|------|------|
| VS2022 Community | ✅ | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
| Windows SDK 10.0.26100.0 | ✅ | Headers + Libs 已安裝，KMDF 1.15-1.35, UMDF 1.9-2.35 |
| WDK 10.1.26100.6584 | ❌ | 登錄有 MSI 記錄但無實際檔案，WDK 資料夾不存在，VS2022 WDK 擴充套件未裝 |

### 待辦

- [ ] 安裝 WDK 10.1.26100（需與 SDK 10.0.26100.0 匹配）
- [ ] 確認 WDK VS2022 擴充套件正確註冊
- [ ] 建立 KMDF 專案骨架

### 備註

WDK 安裝需要单独下載，參考：https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
