# CMSIS-DAP 規格說明

## 概述

CMSIS-DAP (CoreSight Debug Access Port) 是 ARM 提供的開源除錯探針協定，讓開發板可以直接透過 USB 連接到電腦進行除錯和燒錄。

---

## 主要特性

| 特性 | 說明 |
|------|------|
| **協定** | 開源 (Apache 2.0) |
| **介面** | USB → JTAG/SWD |
| **目標** | Cortex-M 全系列 |
| **UART** | 支援 SWO 追蹤 + 虛擬序列埠 |

---

## 硬體介面

### 針腳定義

#### SWD (Serial Wire Debug) - 2 針

| 針腳 | 說明 |
|------|------|
| SWDIO | 雙向資料 |
| SWCLK | 時脈 |

#### JTAG - 5 針

| 針腳 | 說明 |
|------|------|
| TDI | 資料輸入 |
| TDO | 資料輸出 |
| TCK | 時脈 |
| TMS | 模式選擇 |
| TRST | 重設 (可選) |

#### 額外針腳

| 針腳 | 說明 |
|------|------|
| RST | 系統重設 |
| VREF | 目標電壓參考 |
| GND | 地 |

---

## 軟體支援

### 支援的工具

| 工具 | 支援 |
|------|------|
| **pyOCD** | ✅ 完全支援 |
| **OpenOCD** | ✅ |
| **Keil MDK** | ✅ |
| **IAR EWARM** | ✅ |
| **GDB** | ✅ |

### 使用 pyOCD

```bash
# 燒錄
pyocd flash firmware.bin -t M487

# 除錯
pyocd gdbserver

# 列出裝置
pyocd list
```

---

## 產品列表

### 開源/自製

| 產品 | 說明 |
|------|------|
| **DAPLink** | ARM 官方開源韌體 |
| **CMSIS-DAP firmware** | 可下載的參考實作 |
| **Black Magic Probe** | 開源硬體 |

### 商業產品

| 產品 | 說明 |
|------|------|
| **NUCLEO 內建** | ST 開發板附贈 |
| **LPC-Link2** | NXP |
| **Keil ULINK** | Keil 官方 |
| **IAR I-jet** | IAR 官方 |

---

## 通訊協定

### USB 端點

| 端點 | 方向 | 用途 |
|------|------|------|
| EP0 | 控制 | 設定 |
| EP1 | IN | 除錯資料 |
| EP2 | OUT | 除錯命令 |
| EP3 | IN | SWO 追蹤 |

### 指令格式

```
[Packet Size: 2 bytes]
[Request: 1 byte]
[Data: n bytes]
```

### 支援的指令

| 指令 | 說明 |
|------|------|
| DAP_INFO | 取得資訊 |
| DAP_CONNECT | 連線 |
| DAP_DISCONNECT | 離線 |
| DAP_READ_DP | 讀取除錯連接埠 |
| DAP_WRITE_DP | 寫入除錯連接埠 |
| DAP_READ_AP | 讀取存取連接埠 |
| DAP_WRITE_AP | 寫入存取連接埠 |
| DAP_TRANSFER | 傳輸資料 |
| DAP_TRANSFER_BLOCK | 區塊傳輸 |

---

## 與 J-Link/ST-Link 比較

| 特性 | CMSIS-DAP | J-Link | ST-Link |
|------|-----------|--------|---------|
| **價格** | 免費/便宜 | 高 | 中 |
| **開源** | ✅ | ❌ | ❌ |
| **速度** | 中 | 快 | 中 |
| **相容性** | Cortex-M | 全面 | STM32 |
| **軟體支援** | pyOCD/OpenOCD | J-Link 軟體 | ST-Link 軟體 |

---

## DAPLink 韌體

### 特色功能

- USB 磁碟拖放燒錄
- CMSIS-DAP 除錯
- 虛擬序列埠 (CDC)
- SWO 追蹤

### 下載

```bash
# GitHub
https://github.com/ARM-software/CMSIS-DAP
```

---

## 結論

| 適合對象 | 建議 |
|----------|------|
| 學習/開發 | CMSIS-DAP / DAPLink |
| STM32 開發 | ST-Link |
| 專業除錯 | J-Link |
| 大量生產 | 專用燒錄器 |

## 結論

| 適合對象 | 建議 |
|----------|------|
| 學習/開發 | CMSIS-DAP / DAPLink |
| STM32 開發 | ST-Link |
| 專業除錯 | J-Link |
| 大量生產 | 專用燒錄器 |

---

## pyOCD + VS Code 開發環境

### 簡介

pyOCD 是 ARM 開源的除錯工具，支援 CMSIS-DAP，可搭配 VS Code 使用。

### 安裝

```bash
# 安裝 pyOCD
pip install pyocd

# 安裝 Cortex-Debug 擴充 (VS Code)
# 搜尋 "Cortex-Debug" 並安裝
```

### VS Code 設定

#### launch.json

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug with PyOCD",
            "type": "cortex-debug",
            "request": "launch",
            "runToEntryPoint": "main",
            "executable": "${workspaceFolder}/build/firmware.elf",
            "device": "M487",
            "serialPort": "COM3",
            "svdFile": "${workspaceFolder}/M487.svd",
            "pyocdBinary": "pyocd-gdbserver"
        }
    ]
}
```

### 功能清單

- 逐步除錯 (Step/Step Over/Step Out)
- 監看變數 (Watch Variables)
- 暫存器檢視 (Register View)
- 中斷點 (Breakpoints)
- 記憶體檢視 (Memory View)
- 呼叫堆疊 (Call Stack)

### 需要的檔案

| 檔案 | 用途 |
|------|------|
| `.elf` / `.axf` | 編譯後的執行檔 |
| `.svd` | 週邊暫存器定義 |

### 常用指令

```bash
# 燒錄
pyocd flash firmware.bin

# 燒錄並執行
pyocd load firmware.bin -f

# 除錯伺服器
pyocd gdbserver

# 列出支援的晶片
pyocd list --targets

# 抹除晶片
pyocd erase --chip
```

---

## 參考資源

| 資源 | 連結 |
|------|------|
| 官方網站 | https://arm-software.github.io/CMSIS-DAP/latest/ |
| GitHub | https://github.com/ARM-software/CMSIS-DAP |
| pyOCD | https://pyocd.io/ |
| Cortex-Debug | https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug |

---

*建立時間: 2026-03-25*
