# M487_ScsiTool 任務總覽 / Status

> 最後更新：2026-04-05 12:56

---

## 📊 任務狀態摘要

| 狀態 | 數量 |
|------|------|
| ✅ 完成 | 24 |
| 🔄 進行中 | 1 (T001-ST4) |
| ⏳ 待處理 | 4 (T036-T039) |
| ⏳ 待硬體確認 | 1 |

---

## ✅ 已完成 (P0/P1/P2)

| 任務 | 說明 |
|------|------|
| T001 | USB 複合裝置（韌體完成，ICE 已突破）|
| T002-T006 | 編譯環境、OpenOCD、HID Tool |
| T008-T012 | BSP 整合、Review、Toolchain |
| T014-T023 | NACK Retry、Magic Numbers、Buffer、Build 強化 |
| T024-T034 | ITM/SWO、MSC Debug Channel、UART、Flash、Self-Test、GDB、DWT |
| T001-ST1 | 研究 BSP SampleCode ShortPacket KEIL → GCC ✅ |
| T001-ST2 | GCC Makefile 建立 + 編譯 44KB ✅ |
| T001-ST3 | OpenOCD 燒錄 script 建立 ✅ |

---

## 🔄 進行中

| 任務 | 負責 | 說明 |
|------|------|------|
| **T001-ST4** | 🐱 Giroro / 🦀 Kururu | ⏳ 硬體燒錄驗證（等 power-cycle）|

---

## ⏳ 待處理（未開始）

| 任務 | 負責 | 說明 |
|------|------|------|
| T036 | 🐹 Tamama | Unit Test 知識庫（open，需求全未完成）|
| T037 | 🐹 Tamama | 微軟 USB Driver Samples 研究（open，需求全未完成）|
| T038 | 🦀 Kururu | Linux FWUPD 韌體更新知識庫 |
| T039 | 🦀 Dororo | 文件驗收 |

> ⚠️ **修正（2026-04-04）：** STATUS.md 2026-04-03 誤將 T036-T039 標記為 ✅ 完成，實際從未開始。已修正。

---

## ⏸️ 待 Caro 手動確認

- **Power-cycle M487** → 檢查 USB MSC 枚舉（VID=0x0416, PID=0x0470）

---

## 🆕 今日重大發現

| 發現 | 影響 |
|------|------|
| OpenOCD numicro flash driver 不支援 M487（Device ID 0x10004180）| 燒錄需繞道 GDB load |
| GDB load 可燒錄 flash | 燒錄路徑驗證成功 |
| `reset run` 後 FMC remap 未設定 | 需 power-cycle 讓 boot ROM 正確 remap |

---

## Debug 整備矩陣 ✅

| 等級 | 工具 | 狀態 |
|------|------|------|
| L1 | UART Log | ✅ |
| L2 | MSC Debug Channel + CLI | ✅ |
| L2 | hidtool | ✅ |
| L3 | ICE + GDB | ✅ 已驗證 |
| L3 | USB Filter Driver | ⏳ 需 WDK |
| L4 | ITM/SWO + Viewer | ✅ |
| L4 | Flash Error Log | ✅ |
| L4 | Self-Test Mode | ✅ |

---

*T001-ST4 ⏸️ 等 Caro power-cycle。其餘 4 個待處理任務（T036-T039）等待指派執行。*