# Task.md — T038

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T038 |
| 標題 | Linux FWUPD 韌體更新功能建置 |
| 狀態 | ⏳ 待處理 |
| 優先序 | P0 |
| 指派 | **Kururu**（研究 + 分析）|
| 截止 | - |

## 目標

建立公司 IC 支援 Linux FWUPD（Linux Vendor Firmware Update）的完整知識庫，從基礎教學到實際上線

## 涵蓋範圍

### 基礎學習
- [ ] FWUPD 架構與原理
- [ ] LVK（Linux Vendor Firmware Update Kit）介紹
- [ ] DFU（Device Firmware Update）協定基礎
- [ ] UEFI 與 Capsule Update 機制

### 準備階段
- [ ] 公司 IC 的 DFU 協定分析
- [ ] FWUPD plugin 開發前置作業
- [ ] 開發板驗證環境建立

### 實作階段
- [ ] 新增 IC/plugin 開發（以 M487 為例）
- [ ] Firmware CAB 檔案製作
- [ ] LVFS（Firmware Repository）上架流程

### 驗證階段
- [ ] 本地測試（fwupdmgr）
- [ ] LVFS 測試環境驗證
- [ ] 跨發行版驗證（Ubuntu/Debian/Fedora）

### 上線階段
- [ ] 正式 LVFS 上架
- [ ] 企業內部部署方案
- [ ] 安全性考慮（簽章、驗證）

## 知識庫位置

`D:\AiWorkSpace\KM\KM-collect\FWUPD\`

## 備註

**注意：** 需要確認公司目前有哪些 IC 需要支援 FWUPD，以便規劃優先順序
