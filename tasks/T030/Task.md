# Task.md — T030

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T030 |
| 標題 | ITM Trace Viewer（PC 端工具）|
| 狀態 | in_progress |
| 優先序 | P2 |
| 指派 | Tamama |
| 依賴 | T024 完成 |
| 截止 | - |

## 目標

PC 端接收並顯示 ITM SWO trace 資料

## 需求

- [x] ITM/SWO 原理研究（見 T034, itm.c）
- [x] PC 端接收方案分析（UART bridge, J-Link, pyocd）
- [x] Viewer 架構設計
- [x] Python CLI prototype 實作
- [ ] UART-to-USB bridge 硬體驗證（需要實際硬體）
- [ ] J-Link/pyocd 整合驗證

## 產出

| 檔案 | 說明 |
|------|------|
| `tool/itm_trace_viewer.py` | Python CLI prototype（支援 UART/File/JLink 接收）|
| `tool/itm_trace_viewer_README.md` | 使用說明與原理文件 |

## 接收方案比較

| 方案 | 硬體需求 | 難易度 | SWO Baud | 備註 |
|------|---------|--------|----------|------|
| A: UART Bridge | FTDI/CH340 ~$2 | ⭐ 簡單 | 2 MHz max | **推薦首選** |
| B: Nu-Link | Nu-Link debugger | ⭐⭐ 中等 | ? | OpenOCD 支援 |
| C: J-Link | J-Link ~$60+ | ⭐⭐⭐ 簡單 | 10 MHz | 最完整但昂貴 |
| D: pyocd/CMSIS-DAP | CMSIS-DAP ~$10 | ⭐⭐ 中等 | 10 MHz | 開源替代 |

## 已實作功能

- UART-to-USB 接收（pyserial）
- File replay 離線分析
- ITM TPIU frame parser
- Line buffer 重建
- ANSI color 彩色輸出（module-aware）
- ITM packet filtering by port
- HEX dump 模式
- Statistics on exit
