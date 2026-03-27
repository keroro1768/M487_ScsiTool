# Keil MDK 燒錄指南

## 環境需求

| 項目 | 版本/需求 |
|------|-----------|
| Keil MDK | v5.x (Nuvoton Edition) |
| Nuvoton License | 免費 (有效期至 2027/9/25) |
| 目標晶片 | M487JIDAE |
| 調試器 | Nu-Link (SWD) |

---

## 安裝步驟

### 1. 安裝 Keil MDK

從 Nuvoton 官網申請免費 License:
- 網址: https://www.nuvoton.com/tool-and-software/ide-and-compiler/keil-mdk-nuvoton-edition/application-form/
- 填寫表單後會收到 License 檔案

### 2. 安裝 Nuvoton M4 DFP

1. 開啟 Keil μVision
2. `Project` → `Manage` → `Pack Installer`
3. 左邊找到 `Nuvoton` → `M4`
4. 找到 `NuMicro M4_DFP` 點 `Install`
5. 等待下載完成

### 3. 安裝 Nu-Link 驅動

下載並安裝 `Nu-Link_Keil_Driver_V3.22.7946r`:
- 位置: https://www.nuvoton.com/tool-and-software/ide-and-compiler/ (找 Nu-Link Driver)
- 安裝後 USB 驅動會自動設定

---

## 建立新專案

### 1. 新建專案
- `Project` → `New μVision Project...`
- 選擇儲存位置
- 輸入專案名稱

### 2. 選擇晶片
- `Select Device` 對話框
- 搜尋 `M487`
- 選擇 `M487JIDAE`

### 3. 設定 Debug

1. `Project` → `Options for Target` (或 Alt+F7)
2. 切換到 `Debug` 分頁
3. 選 `Cortex-M Target Driver Setup`
4. 確認 `Nuvoton Nu-Link` 被選中
5. 點 `Settings` → `Port` 選 **SWD**
6. 確認有抓到 IDCODE `0x2BA01477`

### 4. 設定 Flash Algorithm

1. `Debug` → `Settings` → `Flash Download`
2. 確認 `NuMicroM4xx FMC` (或類似) 在清單中
3. 大小應為 512KB

---

## 燒錄現有 Binary

### 方式一: 直接下載 (適用於 hex/bin)

1. `File` → `Load Memory Area...`
2. 選擇 `.bin` 檔案
3. 設定起始位址 `0x0`
4. 點 `OK`

### 方式二: 建立空白專案燒錄

1. 建立新專案，選擇 M487JIDAE
2. `Project` → `Options for Target` → `Output`
3. 勾選 `Create HEX File`
4. 編譯 (F7)
5. 下載 (F8 或 Debug → Download)

### 方式三: Keil ULINKpro 命令列

```cmd
"C:\Keil_v5\UV4\UV4.exe" -c Config.ini -d M487 -f Program.hex
```

---

## 燒錄燒選項 Bytes

M487 有 Config 區需要正確設定：

| 位址 | 功能 |
|------|------|
| 0x0030_0000 | Config 0 |
| 0x0030_0004 | Config 1 |
| ... | ... |

可用 OpenOCD 讀取:
```
flash read_bank 0 config.bin 0x00300000 0x20
```

---

## Debug 設定建議

### 時脈設定
- `Debug` → `Settings` → `Clock` 選 **Auto**
- 或手動設 `SWCLK` 頻率

### 燒錄後自動執行
- `Debug` → 勾選 `Run to main()`
- 或燒錄後按 `F5` 執行

---

## 常見問題

| 問題 | 解決 |
|------|------|
| Pack Installer 找不到 Nuvoton | 確認已申請 License |
| 無法下載 | 確認 Nu-Link 驅動正確 |
| 燒錄後程式不執行 | 檢查 vector table 和時脈設定 |
| Debug 連線失敗 | 確認 SWD 線正確連接 |
