# PLAN.md — T030

## 狀態
✅ Core implementation complete (2026-03-28)

## 實作摘要

### ITM/SWO 原理
- M487 PB8 = SWO pin，ARM CoreSight ITM + TPIU
- ITM Stimulus Port 0 用於 log 輸出
- TPIU formatter 包裝 ITM bytes，frame byte 含 source/port info
- SWO baud 可達 2 MHz（Manchester encoding）
- DWT cycle counter 作為 timestamp

### PC 端接收方案（已評估）

**方案 A: UART-to-USB bridge** ✅ 已實作
- 線路：M487 PB8 → FTDI/CH340 → USB
- 支援 baud 115200 ~ 2000000
- pyserial 讀取，成本最低

**方案 B: Nu-Link + OpenOCD**
- OpenOCD 支援 SWO capture
- 需要 Nu-Link 硬體

**方案 C: SEGGER J-Link** ✅ 結構預留
- J-LinkSWOViewer 接收
- 最高 10 MHz SWO baud
- 需 J-Link 硬體

### 架構

```
Receiver (UART/File/JLink)
    → ITMParser (TPIU decode, port demux)
    → LineBuffer (line reconstruction)
    → OutputFormatter (ANSI color, module detection)
```

### 產出

- `tool/itm_trace_viewer.py` — Python CLI (298 lines)
- `tool/itm_trace_viewer_README.md` — 完整文件

## 待驗證

- [ ] UART bridge 硬體實測
- [ ] J-Link SWO 整合
