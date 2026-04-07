# M487_ScsiTool 任務總覽 / Status

> 最後更新：2026-04-07 12:34

---

## 📊 任務狀態摘要

| 狀態 | 數量 |
|------|------|
| ✅ 完成 | 27 |
| 🔄 進行中 | 1 (T001-ST4) |
| ⏸️ 待硬體確認 | 1 |

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
| **T036** | Unit Test 知識庫（Unity/Ceedling/GoogleTest，17 檔，130+ 測試案例）✅ |
| **T037** | Microsoft USB Driver Samples 研究（firefly/USB/HID/Filter 架構）✅ |

---

## 🔄 進行中

| 任務 | 負責 | 說明 |
|------|------|------|
| **T001-ST4** | 🐱 Giroro / 🦀 Kururu | ⏳ 硬體燒錄驗證（等 power-cycle）|

---

## ✅ 完成（知識庫擴展）

| 任務 | 說明 |
|------|------|
| **T036** | Unit Test 知識庫（Unity/Ceedling/GoogleTest，17 檔，130+ 測試案例）✅ |
| **T037** | Microsoft USB Driver Samples 研究（firefly/USB/HID/Filter 架構）✅ |
| **T038** | Linux FWUPD 韌體更新知識庫（22 檔，215KB）✅ |
| **T039** | 文件驗收 ⚠️ CONDITIONAL PASS（3 Major + 5 Minor 待修）|

---

## ⚠️ 待處理（Dororo 發現問題需修復）

| 優先 | 問題 | 位置 |
|------|------|------|
| Major | SET_IDLE 指令格式矛盾（Opcode Table vs 範例）| TRANSLATION.md §1.6 |
| Major | MLX90614 為 SMBus 非 HID-over-I2C，未明確說明 | EXAMPLE.md |
| Major | HID Descriptor wMaxInputLength=4 會截斷資料 | EXAMPLE.md |
| Minor | 內部連結路徑需修正（`../ICE/` → `../../ICE/`）| NuLink 文件 |

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

*T001-ST4 ⏸️ 等 Caro power-cycle。其餘任務（T038/T039 已完成，T039 有 3 Major 待修）。*