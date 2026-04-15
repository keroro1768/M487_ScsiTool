# M487_ScsiTool 任務總覽 / Status

> 最後更新：2026-04-15 14:01

---

## 📊 任務狀態摘要

| 狀態 | 數量 |
|------|------|
| ✅ 完成 | 27 |
| 🔄 進行中 | 1 (T001-ST4) |
| ⏸️ 待硬體確認 | 1 |
| ⏳ 待處理 | 2 (T040, T041) |

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
| T036 | Unit Test 知識庫（Unity/Ceedling/GoogleTest，17 檔，130+ 測試案例）✅ |
| T037 | Microsoft USB Driver Samples 研究（firefly/USB/HID/Filter 架構）✅ |

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
| **T039** | 文件驗收 ✅ 所有 Major 已修復（Minor 建議不阻礙通過）|

---

## ⚠️ 待修復（Dororo 發現，T039 驗收阻礙）

| 優先 | 問題 | 位置 |
|------|------|------|
| ~~**Major**~~ ✅ | SET_IDLE 指令格式矛盾 → **已修復** (`1e22b3b`) | TRANSLATION.md §1.6 |
| ~~**Major**~~ ✅ | MLX90614 為 SMBus 裝置 → **已修復** (新增 ⚠️ 警告聲明) | EXAMPLE.md |
| ~~**Major**~~ ✅ | HID Descriptor `wMaxInputLength=4` → **已修復** (改為 6) | EXAMPLE.md |
| Minor | 內部連結路徑需修正（`../ICE/` → `../../ICE/`）| NuLink 文件 |
| Minor | 外部 URL（Nuvoton 下載連結）需驗證，建議改用 GitHub BSP | M487_Usage_Guide.md |
| Minor | SPEC.md 本地檔案路徑無法驗證 | SPEC.md |

---

## ⏸️ 待 Caro 手動確認

- **Power-cycle M487** → 檢查 USB MSC 枚舉（VID=0x0416, PID=0x0470）

---

## ⏳ 待處理

| 任務 | 負責 | 說明 |
|------|------|------|
| **T040** | 🦀 Dororo | `hid_bridge`/`tool/hidtool_cpp` 文件審查 |
| **T041** | 🦀 Dororo | 建置 + 測試驗證（依賴 T040）|

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

## Enhancement Backlog

| ID | 名稱 | 最後更新 | 狀態 |
|----|------|----------|------|
| E001 | ICE_Breakthrough | 2026-03-30 | 🔄 實施中 |

---

*T001-ST4 ⏸️ 等 Caro power-cycle。T036-T039 知識庫已完成，`hid_bridge/` + `tool/hidtool_cpp/` 已 commit。T040/T041 待 Dororo Review。*
