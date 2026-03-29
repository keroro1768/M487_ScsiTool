# Nu-Link 使用經驗彙整

> 最後更新：2026-03-28
> 來源：GitHub Issues、Nuvoton 論壇、StackOverflow、OpenOCD 官方文件

---

## 1. Nu-Link 型號對照

| 型號 | 介面 | CMSIS-DAP | WebUSB | 支援晶片 |
|------|------|-----------|--------|---------|
| Nu-Link | USB HID | ❌ | ❌ | M0/M4 |
| Nu-Link2-Pro | USB HID + CMSIS-DAP | ✅ | ✅ | M0/M4 |
| Nu-Link3-Pro | USB HID + CMSIS-DAP + WinUSB | ✅ | ✅ | M0/M4 |

> 你手上的型號是 **Nu-Link**（VID=0x0416, PID=0x511C），屬於第一代，**不支援 WebUSB**，但仍可透過 CMSIS-DAP 通訊。

---

## 2. 驅動程式架構（重要觀念）

Nu-Link 在 Windows 上會呈現為 **2 個 USB Interface**：

| Interface | 用途 | 預設驅動 |
|-----------|------|---------|
| **Interface 0** | HID（燒錄/除錯）| Windows 內建 HID |
| **Interface 1** | WinUSB / 通用 USB | Nuvoton WinUSB（oem132.inf）|

```
USB\VID_0416&PID_511C&MI_00  →  HID（系統內建）
USB\VID_0416&PID_511C&MI_01  →  WinUSB（Nuvoton oem132.inf）← OpenOCD 需要這個
```

**OpenOCD 的 cmsis-dap driver 需要存取 Interface 1（WinUSB），並非 HID。**

---

## 3. LIBUSB_ERROR_ACCESS 原因與解法

### 常見原因

**① 驅動未正確安裝（最常見）**
- WinUSB 驅動未安裝，或被其他驅動覆蓋
- 安裝 Keil Nu-Link Driver 時綁定了 HID 驅動而非 WinUSB

**② 權限不足**
- 非管理員執行 OpenOCD
- Windows UAC 阻擋

**③ 另一個程式佔用了裝置**
- Keil MDK 正在背景執行並佔用 Nu-Link
- NuStudio 或其他 Nuvoton 工具正在使用

**④ USB 控制器不相容**
- USB 3.0 控制器與 libusb 可能不相容，嘗試插到 USB 2.0 埠

**⑤ 其他 USB 介面在干擾**
- 有用戶回報：另一個閒置的介面（如 ST Bridge interface）也會造成搶佔，導致 libusb 被拒絕存取

### 解法（依序嘗試）

**Step 1：關閉所有可能佔用 Nu-Link 的程式**
```batch
taskkill /IM UV4.exe /F
taskkill /IM Keil* /F
```

**Step 2：用 Zadig 安裝 WinUSB 驅動**

1. 開啟 Zadig：`D:\AiWorkSpace\M487_ScsiTool\tool\external\zadig-2.9.exe`
2. Options → List All Devices
3. 找到 `NuLink [0416:511C]`
4. 選擇 **WinUSB (v6.x.x.x)** 或 **libusbK**
5. 點擊 **Replace Driver**
6. 出現 UAC 提示 → 點「是」

**Step 3：以系統管理員執行 OpenOCD**
```batch
# 用管理員身份開cmd，再執行
D:\AiWorkSpace\M487_ScsiTool\tool\test_openocd.bat
```

**Step 4：確認所有 Interface 都正確綁定驅動**
- 裝置管理員 → 檢查每個 Interface（MI_00, MI_01）是否都是 WinUSB

---

## 4. OpenOCD + Nu-Link 設定檔注意事項

### cmsis_dap_vid_pid 問題

OpenOCD 預設只取 VID/PID，但 Nu-Link 有多個 Interface。**明確指定 Interface**：

```tcl
# 錯誤：可能抓到錯誤的 interface
cmsis_dap_vid_pid 0x0416 0x511C

# 正確：加上 interface number
cmsis_dap_vid_pid 0x0416 0x511C 0x00   # Interface 0 = HID
cmsis_dap_vid_pid 0x0416 0x511C 0x01   # Interface 1 = WinUSB ← 我們需要這個
```

### 路徑問題

```tcl
# 錯誤：絕對路徑（換機器就壞）
source D:/AiWorkSpace/M487_ScsiTool/tool/openocd/m487_target.cfg

# 正確：使用 OpenOCD 搜尋路徑
source [find tool/openocd/m487_target.cfg]
```

---

## 5. OpenOCD 官方支援狀態

根據 OpenOCD 官方文件（openocd.org）：

> *"Currently supported adapters include the STMicroelectronics ST-LINK, TI ICDI and **Nuvoton Nu-Link**."*

**建議使用的 OpenOCD 版本：**
- **Nuvoton 官方客製化版**：`OpenOCD-Nuvoton-CMSIS-DAP`（你的 `openocd_cmsis-dap.exe`）
- GitHub: https://github.com/OpenNuvoton/OpenOCD-Nuvoton-CMSIS-DAP

---

## 6. Nu-Link 與 Keil MDK 的關係

### 驅動衝突問題

安裝 **Nu-Link_Keil_Driver** 後，會將 Nu-Link 註冊為 Keil 專用裝置，**可能覆蓋 WinUSB 驅動**。

### 解決方案

| 情境 | 驅動選擇 |
|------|---------|
| 只用 **Keil MDK** 燒錄 | 使用 Nuvoton Keil Driver（保持 HID）|
| 只用 **OpenOCD** 燒錄 | 使用 WinUSB（Zadig 安裝）|
| 兩者都需要 | 燒錄前用 Zadig 切換驅動，或燒錄後用 NuStudio 回復 |

### Nu-Link3-Pro 的優勢

Nu-Link3-Pro 支援 **CMSIS-DAP v2**（原生 WinUSB），不需要 Zadig 就能直接用 OpenOCD，且支援 WebUSB 可用於 Keil Studio Desktop/Cloud。

---

## 7. 實務經驗（GitHub / 論壇彙整）

### 問題：CMSIS-DAP command 0x1d not implemented

```
Error: CMSIS-DAP command 0x1d not implemented
Error: CMSIS-DAP command SWD_Sequence failed
```

**原因**：Nu-Link 韌體太舊，不支援某些 SWD 指令。**解法**：更新 Nu-Link 韌體。

---

### 問題：OpenOCD 找不到 CMSIS-DAP 裝置

**檢查清單：**
- [ ] USB 是否正確連接
- [ ] 裝置管理員是否出現 `Nuvoton Nu-Link USB`
- [ ] WinUSB 驅動是否已安裝（Interface 1）
- [ ] 是否被 VMware/USB 網路工具佔用

---

### 問題：LIBUSB_ERROR_NOT_FOUND

**原因**：USB 裝置斷開連接，或驅動未正確安裝。

**解法**：
1. 重新插拔 Nu-Link
2. 確認裝置管理員中無驚嘆號
3. 用 Zadig 重新安裝驅動

---

## 8. 其他替代燒錄工具

| 工具 | 優點 | 缺點 |
|------|------|------|
| **Keil MDK + Nu-Link** | 完整整合 | 需付費 license |
| **NuStudio** | Nuvoton 官方 | 封閉軟體 |
| **OpenOCD + Nu-Link** | 開源、腳本化 | 驅動設定繁瑣 |
| **CMSIS-DAP v2 介面卡** | 原生 WinUSB | 需另外購買 |

---

## 9. 推薦工作流程

### 燒錄時（使用 OpenOCD）

1. 關閉所有 Keil/NuStudio 程式
2. 用 Zadig 確認 WinUSB 驅動正確
3. 以系統管理員執行 OpenOCD
4. 燒錄完成後可隨時切回 Keil

### 若 Zadig 無效

嘗試在 **裝置管理員** 中：
1. 找到 `Nuvoton Nu-Link USB` → Interface 1
2. 滑鼠右鍵 → **更新驅動程式** → **回復驅動程式**

---

## 10. 參考連結

| 主題 | 連結 |
|------|------|
| OpenOCD Nuvoton GitHub | https://github.com/OpenNuvoton/OpenOCD-Nuvoton-CMSIS-DAP |
| Nuvoton Tools GitHub | https://github.com/OpenNuvoton/Nuvoton_Tools |
| Zadig 官方下載 | https://zadig.akeo.ie/ |
| Nu-Link3-Pro WebUSB | https://gitee.com/OpenNuvoton/Nuvoton_Tools/blob/master/README_NuLink2Pro.md |
| OpenOCD Debug Adapter Config | https://openocd.org/doc/html/Debug-Adapter-Configuration.html |
| CMSIS-DAP 驅動安裝 | https://arm-software.github.io/CMSIS-DAP/latest/dap_drv_install.html |
| VisualGDB OpenOCD Troubleshooting | https://visualgdb.com/support/nodevice/ |
