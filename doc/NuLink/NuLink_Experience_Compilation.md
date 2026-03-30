# Nu-Link 使用經驗彙整

> 最後更新：2026-03-30
> 來源：GitHub Issues、Nuvoton 論壇、StackOverflow、OpenOCD 官方文件
> **狀態：已驗證** — 2026-03-30 成功透過 OpenOCD + Nu-Link Debug M487

---

## ⚠️ 重要修正（2026-03-30）

**第一代 Nu-Link（VID=0x0416, PID=0x511C）不使用標準 CMSIS-DAP Protocol！**

| 之前（錯誤）| 之後（正確）|
|-------------|-------------|
| OpenOCD 用 `cmsis-dap` driver | OpenOCD 用 `hla` driver |
| 指定 Interface 1 WinUSB | 指定 `hla layout nulink` |
| `cmsis_dap_vid_pid 0x0416 0x511C` | `hla vid_pid 0x0416 0x511C` |
| 使用 `openocd_cmsis-dap.exe` | 使用 `openocd-build\bin\openocd.exe` |

**詳細原理見：[doc/ICE/01_PROBLEM.md](../ICE/01_PROBLEM.md)**

---

## 1. Nu-Link 型號對照

| 型號 | 介面 | CMSIS-DAP | Proprietary HID | 支援晶片 |
|------|------|-----------|-----------------|---------|
| Nu-Link | USB HID + WinUSB | ❌ | ✅（第一代）| M0/M4 |
| Nu-Link2-Pro | USB HID + CMSIS-DAP | ✅ | ✅ | M0/M4 |
| Nu-Link3-Pro | USB HID + CMSIS-DAP + WinUSB | ✅ | ✅ | M0/M4 |

> 你手上的型號是 **Nu-Link**（VID=0x0416, PID=0x511C），屬於第一代。
> Interface 1 的 WinUSB 界面**只實作 Nuvoton Proprietary HID Protocol**，不是標準 CMSIS-DAP。
> 因此 OpenOCD 必須使用 **`hla` driver**（而非 `cmsis-dap driver`）。

---

## 2. 驅動程式架構（重要觀念）

Nu-Link 在 Windows 上會呈現為 **2 個 USB Interface**：

| Interface | 用途 | 預設驅動 |
|-----------|------|---------|
| **Interface 0 (MI_00)** | HID（燒錄/除錯）| Windows 內建 HID |
| **Interface 1 (MI_01)** | WinUSB / 通用 USB | Nuvoton WinUSB（oem132.inf）|

```
USB\VID_0416&PID_511C&MI_00  →  HID（系統內建）
USB\VID_0416&PID_511C&MI_01  →  WinUSB（Nuvoton oem132.inf）← OpenOCD 對應這個界面，但 Protocol 不是 CMSIS-DAP！
```

**正確觀念：**
- WinUSB 驅動已正確安裝（Interface 1 = WINUSB ✅）
- 但 Interface 1 只實作 **Nuvoton Proprietary HID Protocol**（非標準 CMSIS-DAP）
- 因此 OpenOCD 必須用 **`hla` driver + `hla layout nulink`** 繞過標準 CMSIS-DAP，直接用 Proprietary Protocol

---

## 3. LIBUSB_ERROR_ACCESS 原因與解法

### 已驗證的正確驅動狀態

```
USB\VID_0416&PID_511C&MI_01  →  Status: OK  Driver: WINUSB ✅
```

**此時仍出現 LIBUSB_ERROR_ACCESS 的原因：使用了錯誤的 OpenOCD driver**

| 錯誤做法 | 正確做法 |
|---------|---------|
| `adapter driver cmsis-dap` | `adapter driver hla` |
| `cmsis_dap_vid_pid 0x0416 0x511C` | `hla vid_pid 0x0416 0x511C` + `hla layout nulink` |
| `openocd_cmsis-dap.exe` | `openocd-build\bin\openocd.exe` |

### 其他可能原因

**① 權限不足**
- 非管理員執行 OpenOCD
- Windows UAC 阻擋

**② 另一個程式佔用了裝置**
- Keil MDK 正在背景執行並佔用 Nu-Link
- NuStudio 或其他 Nuvoton 工具正在使用

**③ USB 控制器不相容**
- USB 3.0 控制器與 libusb 可能不相容，嘗試插到 USB 2.0 埠

### 解法（依序嘗試）

**Step 1：關閉所有可能佔用 Nu-Link 的程式**
```batch
taskkill /IM UV4.exe /F
taskkill /IM Keil* /F
```

**Step 2：使用正確的 OpenOCD + Wrapper（推薦）**

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "init" -c "targets" -c "shutdown"
```

Wrapper (`openocd.bat`) 會自動設定 MSYS2 DLL PATH，呼叫 `openocd-build\bin\openocd.exe`。

**Step 3：手動設定 PATH（如 wrapper 無效）**
```powershell
$env:PATH = 'C:\msys64\mingw64\bin;' + $env:PATH
cd D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD-Nuvoton\OpenOCD\bin
.\openocd.exe -s ../scripts -f D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg
```

**Step 4：確認 WinUSB 驅動（如仍失敗）**
1. 開啟 Zadig：`D:\AiWorkSpace\M487_ScsiTool\tool\external\zadig-2.9.exe`
2. Options → List All Devices
3. 找到 `NuLink [0416:511C]`
4. 選擇 **WinUSB (v6.x.x.x)** 或 **libusbK**
5. 點擊 **Replace Driver**

---

## 4. OpenOCD + Nu-Link 設定檔注意事項

### ✅ 正確的 OpenOCD 命令序列

```tcl
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd
swd newdap M487 cpu -expected-id 0x2BA01477
dap create M487.dap -chain-position M487.cpu
target create M487.cpu cortex_m -dap M487.dap
adapter speed 4000
```

完整設定檔已驗證可用：`tool/openocd/nulink_m487_ice.cfg`

### ❌ 錯誤的做法（會失敗）

```tcl
# 錯誤 1：用 cmsis-dap driver → CMD_INFO 收到 STALL
adapter driver cmsis-dap
cmsis_dap_vid_pid 0x0416 0x511C

# 錯誤 2：用 hla 但不指定 layout
adapter driver hla
hla vid_pid 0x0416 0x511C
# → Error: No adapter layout 'nulink' found

# 錯誤 3：用 openocd_cmsis-dap.exe
# → hla layout nulink 指令無效
```

### 路徑問題

```tcl
# 錯誤：相對路徑（[find] 可能找不到）
source [find tool/openocd/m487_target.cfg]

# 正確：使用絕對路徑
source D:/AiWorkSpace/M487_ScsiTool/tool/openocd/nulink_m487_ice.cfg
```

---

## 5. OpenOCD 官方支援狀態

> ⚠️ OpenOCD 的 `hla nulink` driver 是在 Nuvoton 客製化 build 中支援，而非上游 OpenOCD。

根據實際測試（2026-03-30）：

| OpenOCD Build | 版本 | NULINK HLA | 可用性 |
|---------------|------|-------------|--------|
| `openocd-build\bin\openocd.exe` | 0.12.0+dev (2026-03-25) | ✅ 完整 | ✅ **本機可用** |
| `OpenOCD-Nuvoton\bin\openocd_cmsis-dap.exe` | 0.12.0 (2025-02-17) | ❌ | ❌ HLA 初始化失敗 |
| `Tool\openocd\OpenOCD-20260302-0.12.0\bin\openocd.exe` | 0.12.0 (sysprogs) | ❌ | ❌ 無 NULINK layout |

**成功驗證的完整設定見：[doc/ICE/02_SOLUTION.md](../ICE/02_SOLUTION.md)**

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

**原因**：Nu-Link 韌體太舊，不支援某些 SWD 指令。**解法**：更新 Nu-Link 韌體，或確認使用的是 `hla` driver 而非 `cmsis-dap driver`。

---

### 問題：OpenOCD 找不到 CMSIS-DAP 裝置

**檢查清單：**
- [ ] USB 是否正確連接
- [ ] 裝置管理員是否出現 `Nuvoton Nu-Link USB`
- [ ] WinUSB 驅動是否已安裝（Interface 1）
- [ ] **是否使用了正確的 OpenOCD build（openocd-build）和 `hla` driver**

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
| **OpenOCD + Nu-Link** | 開源、腳本化、GDB Debug | **需用 `hla` driver** |
| **CMSIS-DAP v2 介面卡** | 原生 WinUSB | 需另外購買 |

---

## 9. 推薦工作流程

### 燒錄時（使用 OpenOCD）

1. 關閉所有 Keil/NuStudio 程式
2. 使用 `openocd.bat` 執行 OpenOCD（自動設定 MSYS2 DLL PATH）
3. 燒錄完成後可隨時切回 Keil

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
| **M487 ICE 解決方案** | [doc/ICE/QUICK_START.md](../ICE/QUICK_START.md) |
| **ICE 問題分析** | [doc/ICE/01_PROBLEM.md](../ICE/01_PROBLEM.md) |
