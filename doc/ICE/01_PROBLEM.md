# M487 ICE 連線問題分析

> 日期：2026-03-30  
> 狀態：✅ 已解決

---

## 🔍 問題現象

### 錯誤訊息

```
Error: CMSIS-DAP command mismatch. Sent 0x0 received 0x80
Error: CMSIS-DAP command CMD_INFO failed.
```

```
Error: unable to find a matching CMSIS-DAP device
```

```
Error: LIBUSB_ERROR_ACCESS
```

---

## 🏥 根本原因分析

### 第一代 Nu-Link USB 架構

第一代 Nu-Link（VID=0x0416, PID=0x511C）在 Windows 上呈現為 **2 個 USB Interface**：

```
USB Composite Device (VID=0x0416, PID=0x511C)
├── Interface 0 (MI_00): HID Interface
│   ├── Driver: Windows 內建 HID (hidclass.sys)
│   └── 用途: Keil/NuStudio 原生燒錄工具
└── Interface 1 (MI_01): WinUSB Interface
    ├── Driver: WinUSB ✅（已正確安裝）
    ├── 預期用途: OpenOCD CMSIS-DAP
    └── 實際行為: Proprietary HID Protocol ❌
```

### 關鍵發現：Interface 1 不實作標準 CMSIS-DAP

| 測試 | 命令 | 回應 | 結論 |
|------|------|------|------|
| CMSIS-DAP CMD_INFO (0x00) | `cmsis_dap_vid_pid 0x0416 0x511C` | `0x80 STALL` | ❌ 非標準 CMSIS-DAP |
| WinUSB Driver | 登錄確認 | `Service = WINUSB` | ✅ 驅動正常 |
| Protocol 分析 | nulink_usb.c 原始碼 | Proprietary HID commands | ✅ 確認原因 |

**結論：Interface 1 的 WinUSB 界面只實作 Nuvoton proprietary HID protocol，不是標準 CMSIS-DAP。**

---

## 🔬 三個 OpenOCD Build 測試結果

| Build | 版本 | 大小 | CMSIS-DAP | NULINK HLA | 可用 |
|-------|------|------|-----------|------------|------|
| `openocd-build\bin\openocd.exe` | **0.12.0+dev (2026-03-25)** | 19MB | ✅ | ✅ 完整 | ✅ **本機可用** |
| `OpenOCD-Nuvoton\bin\openocd_cmsis-dap.exe` | 0.12.0 (2025-02-17) | 16MB | ✅ | ❌ `hla layout` 指令無效 | ❌ |
| `Tool\openocd\OpenOCD-20260302-0.12.0\bin\openocd.exe` (sysprogs) | 0.12.0 (2026-03-02) | 5MB | ✅ | ❌ 無 NULINK layout | ❌ |
| `openocd-build\bin\openocd.exe` (無 MSYS2 PATH) | 0.12.0+dev | 19MB | ❌ | ✅ | ❌ DLL 缺失 |

### 為何 openocd_cmsis-dap.exe 的 `hla layout nulink` 無效？

`openocd_cmsis-dap.exe` 是 Nuvoton 客製化版本，預設綁定 CMSIS-DAP HID 底層。雖然 `hla` 驅動有列出，但 `nulink` layout 的初始化未正確執行，導致 `hla layout nulink` 指令回應 `invalid command name`。

### 為何 sysprogs OpenOCD 無 NULINK layout？

sysprogs 的 OpenOCD build 編譯時未啟用 `--enable-nulink` 或 `HLADAPTER_NULINK`，導致 `nulink` layout 不在可用清單中。

---

## 📊 Nuvoton Proprietary HID Protocol vs CMSIS-DAP

### Nuvoton Nu-Link 專有指令（來自 nulink_usb.c）

| Command | Opcode | 用途 |
|---------|--------|------|
| `CMD_CHECK_ID` | 0xA3 | 檢查晶片 ID |
| `CMD_READ_REG` | 0xB5 | 讀取 CPU 暫存器 |
| `CMD_WRITE_REG` | 0xB8 | 寫入 CPU 暫存器 |
| `CMD_READ_RAM` | 0xB1 | 讀取 RAM |
| `CMD_WRITE_RAM` | 0xB9 | 寫入 RAM |
| `CMD_MCU_RESET` | 0xE2 | 重置 MCU |
| `CMD_MCU_STOP_RUN` | 0xD2 | 停止 MCU |
| `CMD_MCU_FREE_RUN` | 0xD3 | 自由運行 |
| `CMD_SET_CONFIG` | 0xA2 | 設定配置 |

### 標準 CMSIS-DAP Command（失敗）

| Command | Opcode | 用途 |
|---------|--------|------|
| `CMD_INFO` | 0x00 | 獲取介面資訊（→ STALL ❌）|
| `CMD_CONNECT` | 0x01 | 連接調試器 |
| `CMD_DISCONNECT` | 0x02 | 斷開連接 |

---

## ⚠️ 為何之前 LIBUSB_ERROR_ACCESS？

### 原因鏈

1. 使用 `openocd_cmsis-dap.exe` + `cmsis_dap_vid_pid`
2. Interface 1 是 WinUSB，OpenOCD 嘗試開啟
3. WinUSB 介面打開成功 → 發送 CMSIS-DAP CMD_INFO
4. 設備回 0x80 (STALL) → OpenOCD 認為是 CMSIS-DAP 但失敗
5. 或：設備被其他進程佔用 → LIBUSB_ERROR_ACCESS

### 解決方案

使用 `openocd-build` + `hla driver` + `hla layout nulink`，直接繞過 CMSIS-DAP，使用 proprietary protocol。

---

## 📋 驅動狀態確認（已驗證）

```powershell
# Interface 1 驅動已正確
Get-PnpDevice | Where-Object { $_.DeviceId -match '0416.*511C' }
```

結果：
```
USB\VID_0416&PID_511C&MI_01\...  Status: OK  (WinUSB ✅)
USB\VID_0416&PID_511C&MI_00\...  Status: OK  (HID ✅)
```

---

*最後更新：2026-03-30 11:30*
