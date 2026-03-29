# PLAN.md — T038

## 執行計畫

### Phase 1：基礎學習（預估 1-2 天）

#### Step 1.1：FWUPD 架構研究
- FWUPD 官方網站：https://fwupd.org/
- GitHub：https://github.com/fwupd
- Architecture 文件
- Plugin 開發文件

#### Step 1.2：LVK 研究
- LVK（Linux Vendor Firmware Update Kit）介紹
- 與 FWUPD 的關係
- 使用情境

#### Step 1.3：DFU 協定基礎
- USB DFU 協定（DFU 1.1）
- 與 FWUPD 的整合方式

### Phase 2：準備階段（預估 2-3 天）

#### Step 2.1：公司 IC 分析
- 確認需要支援的 IC 型號
- 分析現有的 bootloader/DFU 能力
- 評估需要哪些硬體修改

#### Step 2.2：M487 驗證環境
- 以 M487 作為第一個驗證 IC
- 建立開發板測試環境

#### Step 2.3：Plugin 開發前置
- FWUPD plugin 開發環境
- meson build system

### Phase 3：實作階段（預估 3-5 天）

#### Step 3.1：Plugin 開發
- 建立公司的 FWUPD plugin
- 實作 IC 的 DFU 介面
- 與 M487 DFU 整合

#### Step 3.2：CAB 檔案製作
- Firmware metadata 定義
- `.metainfo.xml` 製作
- 簽章流程

#### Step 3.3：LVFS 上架
- LVFS 帳號申請
- 測試環境上架
- 正式上架流程

### Phase 4：驗證階段（預估 2-3 天）

#### Step 4.1：本地測試
```bash
fwupdmgr install firmware.cab
fwupdmgr get-devices
fwupdmgr history
```

#### Step 4.2：跨平台驗證
- Ubuntu 22.04+
- Debian 12+
- Fedora 38+

### Phase 5：上線階段（預估 2-3 天）

#### Step 5.1：正式上架
- LVFS 正式發布
- SHA256 簽章驗證

#### Step 5.2：企業部署
- 內部 firmware repository
- 自動化更新腳本
- 安全性政策

## 預估總工時

10-18 天

## 參考資源

- FWUPD 官網：https://fwupd.org/
- LVFS：https://fwupd.org/lvfs/
- FWUPD GitHub：https://github.com/fwupd
- Plugin 開發文件：https://fwupd.org/swud.html
- LVK GitHub：https://github.com/hughsie/lvk
