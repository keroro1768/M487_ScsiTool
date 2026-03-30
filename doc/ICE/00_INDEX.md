# M487 ICE 連線系統文檔索引

> 整理時間：2026-03-30  
> 狀態：✅ 已完全解決

---

## 📋 文檔總覽

| 文件 | 說明 |
|------|------|
| **[QUICK_START.md](QUICK_START.md)** | 🚀 **快速上手** — 第一次使用看這篇（燒錄 + Debug + 驗證）|
| **[01_PROBLEM.md](01_PROBLEM.md)** | 問題分析：LIBUSB_ERROR_ACCESS 根本原因 |
| **[02_SOLUTION.md](02_SOLUTION.md)** | 解決方案：成功配置與快速啟動指南 |
| **[03_VSCODE.md](03_VSCODE.md)** | VSCode Debug 整合：launch.json 完整設定 |
| **[04_VERIFICATION.md](04_VERIFICATION.md)** | 驗證方法：連線測試與預期輸出 |

---

## 🎯 閱讀順序

```
第一次使用 → [QUICK_START.md] ← 快速燒錄、Debug、驗證
需要原理 → [01_PROBLEM.md]   ← LIBUSB / CMSIS-DAP / NULINK Protocol
需要設定 → [02_SOLUTION.md]  ← OpenOCD 設定檔 + 技術細節
VSCode    → [03_VSCODE.md]   ← launch.json 設定
驗證     → [04_VERIFICATION.md] ← 預期輸出 + 錯誤排除
```

---

## 🚀 快速鏈接

**🚀 立即燒錄 + Debug：**
- [快速燒錄（CLI）](QUICK_START.md#1-燒錄韌體)
- [VSCode F5 Debug](QUICK_START.md#2-debugvscode)
- [驗證 ICE 連線](QUICK_START.md#3-驗證-ice-連線)

**🔍 問題排查：**
- [LIBUSB_ERROR_ACCESS](01_PROBLEM.md#libusb_error_access)
- [CMSIS-DAP vs NULINK Protocol](01_PROBLEM.md#cmsis-dap-vs-nulink)
- [OpenOCD Build 比較](01_PROBLEM.md#三個-openocd-build)
- [常見問題](QUICK_START.md#️-常見問題)

---

## 📌 關鍵發現摘要

### ✅ 已解決：第一代 Nu-Link + OpenOCD

| 項目 | 值 |
|------|---|
| **OpenOCD Build** | `openocd-build\bin\openocd.exe` (0.12.0+dev 2026-03-25) |
| **Driver** | `hla` (HLA, not CMSIS-DAP) |
| **Protocol** | Proprietary HID (非標準 CMSIS-DAP) |
| **M487 IDCODE** | `0x2BA01477` |
| **Speed** | 4MHz SWD |
| **Breakpoints** | 6 個 |
| **Watchpoints** | 4 個 |

### ⚠️ 重要：這些 Build 無法使用

| Build | 原因 |
|-------|------|
| `OpenOCD-Nuvoton\bin\openocd_cmsis-dap.exe` | NULINK HLA 指令未正確初始化 |
| `Tool\openocd\OpenOCD-20260302-0.12.0\bin\openocd.exe` (sysprogs) | 無 NULINK layout |
| `openocd-build\bin\openocd.exe` (無 MSYS2 PATH) | 缺少 MSYS2 DLL |

---

## 📁 目錄結構

```
D:\AiWorkSpace\M487_ScsiTool\
└── doc\
    └── ICE\
        ├── 00_INDEX.md           ← 本文件
        ├── QUICK_START.md        ← 🚀 快速上手（新手起點）
        ├── 01_PROBLEM.md         ← 問題分析
        ├── 02_SOLUTION.md        ← 解決方案
        ├── 03_VSCODE.md         ← VSCode Debug 整合
        └── 04_VERIFICATION.md    ← 驗證方法
```

---

## 🔗 相關文檔

- [VSCode OpenOCD GDB IDE Setup](../IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md)
- [Nu-Link 使用經驗彙整](../NuLink/NuLink_Experience_Compilation.md)
- [KeroroTeam 共享情報](../../KeroroTeam/knowledge/hardware/ICE-M487/00_OVERVIEW.md)

---

*最後更新：2026-03-30 11:41*
