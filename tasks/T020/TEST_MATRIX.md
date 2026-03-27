# T020 — 硬體到貨後測試矩陣

> **目的：** 硬體到貨時立即執行，不浪費一分鐘  
> **建立日期：** 2026-03-27  
> **負責人：** Kururu（PM）+ Dororo（硬體測試）

---

## 📁 資料夾結構

```
T020/
├── TEST_MATRIX.md          # 本文件
├── logs/                   # 測試執行日誌
│   └── YYYYMMDD_HHMMSS.txt
├── test_data/              # 測試資料（指令腳本、hex檔）
│   ├── flash_firmware.bat
│   └── usb_enumeration_test.ps1
└── reports/                # 測試報告產出
    └── TEST_REPORT_YYYYMMDD.md
```

---

## ⚡ 測試矩陣總覽

| # | 測試項目 | 預期時間 | 失敗應變 | 負責人 |
|---|---------|---------|---------|--------|
| 1 | USB 枚舉（VID=0x04F3 PID=0x0732） | 30 min | 檢查 D+/D- 線路、供電 | Dororo |
| 2 | HID I2C 通訊 | 1 hr | 驗證 T004 端點配置 | Dororo |
| 3 | MSC RAM Disk 讀寫 | 1 hr | 檢查 USB descriptors | Dororo |
| 4 | 複合裝置同時運作 | 2 hr | 測試端點衝突 | Dororo |
| 5 | I2C_Read 功能驗證 | 1 hr | 確認 NACK retry 行為 | Dororo |
| 6 | I2C Bus Scan | 30 min | 確認接線與設備地址 | Dororo |

**總預期時間：** ~6 小時

---

## 🔬 測試項目 1：USB 枚舉（主機辨識 VID=0x04F3 PID=0x0732）

### 測試步驟（Step by Step）

1. 將 M487 開發板連接 USB 線至 Windows 主機
2. 確認開發板供電正常（LED 亮起）
3. 開啟 Windows 裝置管理員
4. 等待 10 秒，觀察是否有新裝置出現
5. 若出現「未知裝置」或驚嘆號，展開查看內容
6. 確認 VID=0x04F3, PID=0x0732

### 預期結果

- 裝置管理員出現「M487 Composite Device」或類似名稱
- 無驚嘆號或錯誤標記
- USB 描述碼正確：VID=0x04F3, PID=0x0732

### 失敗時檢查點（Checklist）

- [ ] 確認 USB 纜線為 data-capable（非僅充電線）
- [ ] 確認 D+ / D- 線路未短路或斷路
- [ ] 確認 USB 供電 5V 正常（量測 VBUS）
- [ ] 確認開發板端 USB 接口焊接良好
- [ ] 更換 USB 埠（可能前端 USB 供電不足）
- [ ] 更換 USB 纜線
- [ ] 重新燒錄 firmware（可能 firmware crash）

### 測試資料/指令

```powershell
# PowerShell: 列出所有 USB 裝置
Get-PnpDevice -InstanceId "USB*" | Format-Table InstanceId, Status, FriendlyName

# 查找特定 VID/PID
Get-PnpDevice -InstanceId "USB*" | Where-Object { $_.FriendlyName -match "0416|5020" }
```

---

## 🔬 測試項目 2：HID I2C 通訊

### 測試步驟（Step by Step）

1. 確認 USB 枚舉測試通過
2. 開啟 Windows HID API 测试工具（或使用 `hidapitester`）
3. 發送 HID GetReport 請求
4. 發送 HID SetReport（I2C Write command）
5. 接收 HID Input Report（I2C Read response）
6. 記錄通訊延遲時間

### 預期結果

- HID 報告可正常發送/接收
- I2C Write → Read 完整 transaction 完成
- 無 timeout 或 NACK 錯誤

### 失敗時檢查點（Checklist）

- [ ] 確認 T004 端點配置正確（EP1 IN, EP2 OUT）
- [ ] 確認 HID Descriptor 格式符合 HID-over-I2C 規範
- [ ] 確認 I2C 設備地址正確
- [ ] 確認 I2C SCL/SDA 線路完整性
- [ ] 使用邏輯分析儀觀察 I2C 波形
- [ ] 檢查 firmware log（UART debug output）

### 測試資料/指令

```bash
# hidapitester 指令範例（需預先安裝）
hidapitester --vidpid 0416/5020 --open --sendfeature 0x00 --data "02 10 00 00 04 00"
hidapitester --vidpid 0416/5020 --open --getinputreport --report 1
```

---

## 🔬 測試項目 3：MSC RAM Disk 讀寫

### 測試步驟（Step by Step）

1. 確認 USB 枚舉測試通過
2. 系統應出現新的磁碟機（如 E:）
3. 開啟檔案總管，確認磁碟容量（約 18KB RAM Disk）
4. 建立測試檔案：`echo "Test Data" > E:\test.txt`
5. 讀取測試檔案：`type E:\test.txt`
6. 刪除測試檔案：`del E:\test.txt`
7. 執行大容量寫入測試

### 預期結果

- RAM Disk 出現在檔案總管
- 容量正確（約 18KB）
- 讀寫操作成功，無錯誤
- 寫入後可正常讀取相同內容

### 失敗時檢查點（Checklist）

- [ ] 確認 USB descriptors 中 MSC class 設定正確
- [ ] 確認端點配置（通常 EP3 Bulk IN/OUT）
- [ ] 檢查 SCSI INQUIRY response
- [ ] 檢查 USB Mass Storage 描述碼
- [ ] 更換 USB 埠測試
- [ ] 在 Linux/macOS 交叉測試（排除主機問題）

### 測試資料/指令

```powershell
# 查看所有磁碟機
Get-PSDrive -PSProvider FileSystem

# 格式化後測試（警告：會清除資料）
# Format-Volume -DriveLetter E -FileSystem FAT32

# 效能測試
Measure-Command { Copy-Item test.bin E:\ }
```

---

## 🔬 測試項目 4：複合裝置同時運作

### 測試步驟（Step by Step）

1. 確認 MSC 和 HID 皆可單獨運作
2. 同時開啟 HID 測試工具和檔案總管
3. 一邊執行 HID I2C 讀寫
4. 一邊執行 MSC 檔案讀寫
5. 觀察兩者是否相互影響
6. 測試高負載情境（HID 高速輪詢 + MSC 檔案傳輸）

### 預期結果

- MSC 和 HID 同時運作無衝突
- 兩者皆可達到預期效能
- 無資料錯誤或裝置斷線

### 失敗時檢查點（Checklist）

- [ ] 檢查端點位址是否衝突（MSC EP3 vs HID EP1/EP2）
- [ ] 確認 USB bandwidth 足夠
- [ ] 檢查 DMA 通道是否衝突
- [ ] 確認中斷優先權設定
- [ ] 檢查 firmware 中 critical section 保護
- [ ] 使用 USB Protocol Analyzer 觀察匯流排

### 測試資料/指令

```powershell
# 同時測試腳本（建議寫成 .ps1）
Start-Job -ScriptBlock { 
    # MSC 寫入壓力測試
    for ($i=1; $i -le 100; $i++) { 
        Copy-Item "largefile.bin" E:\ 
        Remove-Item "E:\largefile.bin" 
    }
}
Start-Job -ScriptBlock { 
    # HID 高速輪詢
    while ($true) { 
        .\hidapitester --vidpid 0416/5020 --open --getinputreport --report 1
        Start-Sleep -Milliseconds 10
    }
}
```

---

## 🔬 測試項目 5：I2C_Read 功能驗證

### 測試步驟（Step by Step）

1. 確認 I2C Bus Scan 完成並找到目標裝置
2. 使用 HID 工具發送 I2C Write（目標位址 + Register 位址）
3. 接收 I2C Read 回應
4. 確認資料內容正確
5. 測試不同資料長度（1 byte ~ 64 bytes）
6. 測試連續讀取（連續 10 次 transaction）

### 預期結果

- I2C Read 可正確讀取目標裝置暫存器
- 資料內容與預期一致
- 無單次失敗即中断的問題
- NACK 時有 retry 機制

### 失敗時檢查點（Checklist）

- [ ] 確認 NACK retry 行為（T014 任務修復重點）
- [ ] 確認 I2C Read 函式無截斷風險（T021）
- [ ] 確認 buffer 邊界保護（T022）
- [ ] 使用邏輯分析儀觀察 I2C NACK 之後的 retry
- [ ] 檢查 firmware log 中的 retry 次數

### 測試資料/指令

```bash
# 測試 I2C Read 不同長度
hidapitester --vidpid 0416/5020 --open --sendfeature 0x00 --data "02 10 00 00 01 00"  # 讀1 byte
hidapitester --vidpid 0416/5020 --open --sendfeature 0x00 --data "02 10 00 00 10 00"  # 讀16 bytes
hidapitester --vidpid 0416/5020 --open --sendfeature 0x00 --data "02 10 00 00 40 00"  # 讀64 bytes
```

---

## 🔬 測試項目 6：I2C Bus Scan

### 測試步驟（Step by Step）

1. 確認 I2C 設備連接正確（SCL → PE2, SDA → PE3）
2. 燒錄 I2C Bus Scan firmware（或使用 UART 命令觸發）
3. 執行 I2C Bus Scan（掃描 0x00 ~ 0x7F）
4. 記錄找到的設備位址
5. 確認目標設備位址出現在列表中

### 預期結果

- Bus Scan 完成，無 hang 或 crash
- 找到所有預期連接的 I2C 設備
- 回傳的設備位址列表正確

### 失敗時檢查點（Checklist）

- [ ] 確認 SCL（PE2）和 SDA（PE3）線路連接正確
- [ ] 確認 I2C 設備供電正常
- [ ] 確認目標設備位址（檢查 datasheet 或dip開關）
- [ ] 確認上拉電阻已安裝（通常 4.7kΩ）
- [ ] 使用萬用表量測 SCL/SDA 電壓（應為 3.3V）
- [ ] 使用邏輯分析儀確認 I2C 時序

### 測試資料/指令

```bash
# 預期設備位址（請根據實際硬體調整）
# 例如：EEPROM 0x50, Sensor 0x68, 顯示器 0x3C

# UART 指令範例（若 firmware 支援）
# 發送: "i2c scan\r\n"
# 預期回應: "Found devices: 0x50, 0x68\r\n"
```

---

## 📊 測試報告模板

```markdown
# 測試報告 — YYYYMMDD

## 環境
- 測試日期：
- 測試人員：
- 硬體版本：
- 韌體版本：

## 測試結果

| 測試項目 | 結果 | 備註 |
|---------|------|------|
| USB 枚舉 | ✅ PASS / ❌ FAIL | |
| HID I2C 通訊 | ✅ PASS / ❌ FAIL | |
| MSC RAM Disk | ✅ PASS / ❌ FAIL | |
| 複合裝置 | ✅ PASS / ❌ FAIL | |
| I2C_Read | ✅ PASS / ❌ FAIL | |
| I2C Bus Scan | ✅ PASS / ❌ FAIL | |

## 失敗項目細節

### 失敗項目：XXX
- 失敗時間：
- 失敗現象：
- 已嘗試的解決方案：
- 根本原因（推測）：

## 測試日誌

（請貼上 logs/ 目錄中的測試日誌）
```

---

## 🚨 緊急聯絡

- **Dororo（硬體測試）：** 負責實體接線、儀器操作
- **Giroro（韌體）：** 負責 firmware 問題分析
- **Kururu（PM）：** 協調資源、進度追蹤

---

## 📋 前置準備檢查清單

硬體到貨前請確認以下項目已完成：

- [ ] M487 開發板已準備
- [ ] USB 纜線（資料傳輸型，非僅充電）
- [ ] USB 邏輯分析儀（建議）
- [ ] 萬用表
- [ ] 韌體已燒錄最新版本
- [ ] 测试工具已安裝（hidapitester, PowerShell scripts）
- [ ] 測試資料夾結構已建立
