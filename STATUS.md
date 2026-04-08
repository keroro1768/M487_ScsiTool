# M487_ScsiTool 任務總覽 / STATUS

> 更新：2026-04-09 06:58 (Asia/Taipei)

## 任務狀態摘要

| 任務 | 狀態 | 優先 | 阻礙 |
|------|------|------|------|
| T001 — USB 複合裝置 | ⏸️ 待硬體 | P0 | 等 Caro power-cycle |
| T002-T005 | ✅ Finish | - | |
| T006 | ⏰ Pending | P2 | 低優先 |
| T007 | ⏰ Pending | - | 待 T001 完成 |
| T008-T009 | ✅/N/A | - | |
| T010 | ⏸️ 待硬體 | P1 | 等 T001 |
| T011-T034 | ✅/Framework | P0-P2 | |
| T035 | 📖 Reference | P3 | |

## Debug 整備矩陣

| Lv | 工具 | Task | 狀態 |
|----|------|------|------|
| L0 | 產品出廠 | - | 無 Debug |
| L1 | UART Log | T028 | ✅ |
| L2 | MSC Debug Channel | T025 | ✅ |
| L2 | MSC Debug CLI | T026 | ✅ |
| L2 | hidtool | T029 | ✅ |
| L3 | USB Filter Driver | T027 | ⏳ 需 WDK |
| L3 | GDB RSP Server | T033 | ✅ Framework |
| L4 | ICE + GDB (Nu-Link) | T001 | ⏸️ 等硬體 |
| L4 | ITM/SWO | T024 | ✅ |
| L4 | ITM Viewer | T030 | ✅ |
| L4 | DWT Debug | T034 | ✅ Framework |
| L4 | Flash Error Log | T031 | ✅ |
| L4 | Self-Test Mode | T032 | ✅ |

## 待 Caro 動作

- **T001-ST4**: Power-cycle M487 → 確認 USB MSC 枚舉（VID=0x04F3 PID=0x0732）

## Enhancement Backlog

| 名稱 | 狀態 | 年齡 | 說明 |
|------|------|------|------|
| E001_ICE_Breakthrough | ⏳ Pending | ~10d | 未超過 30 天限制 |

---
*自動產生 by Heartbeat"