# Task.md — T002

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T002 |
| 標題 | arm-none-eabi-gcc 編譯環境建立 |
| 狀態 | done |
| 優先序 | - |
| 指派 | Giroro |
| 依賴 | - |
| 截止 | - |

## 目標

建立 M487 韌體的 GCC 編譯環境

## 需求

- [x] xpack-arm-none-eabi-gcc 15.2.1-1.1 安裝
- [x] Makefile 建立（VENDOR_LBK + Composite）
- [x] 編譯成功驗證

## 進度

**工具鏈：**
- Compiler: xpack-arm-none-eabi-gcc 15.2.1-1.1
- Location: `C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\`

**包含驅動：** hsusbd, usbd, usci_i2c, clk, gpio, sys, pdma, fmc, uart, retarget, system_M480

## 備註

-

## 原始 README.md

已遷移至 `firmware/composite/build_gcc/` Makefile 相關位置
