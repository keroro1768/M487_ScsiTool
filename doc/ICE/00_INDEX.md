# M487 ICE 連線系統文檔索引

> 整理時間：2026-03-30  
> 狀態：✅ 已完全解決

---

## 📋 文檔總覽

| 文件 | 說明 |
|------|------|
| **[QUICK_START.md](QUICK_START.md)** | 🚀 **快速上手** — 第一次使用看這篇（燒錄 + Debug + 驗證）|
| **[01_PROBLEM.md](01_PROBLEM.md)** | 問題分析：LIBUSB_ERROR_ACCESS 根本原因 |
| **[02_SOLUTION.md](02_SOLUTION.md)** | 解決方案：成功配置、OpenOCD 設定、GDB 指令 |
| **[03_VSCODE.md](03_VSCODE.md)** | VSCode Debug 整合：launch.json 完整設定 |
| **[04_VERIFICATION.md](04_VERIFICATION.md)** | 驗證方法：預期輸出 + Debug 矩陣 |
| **[05_GDB_DEBUG_TEST.md](05_GDB_DEBUG_TEST.md)** | 🎯 **GDB Debug 實戰驗證** — 單步/斷點/FreeRun/記憶體/觀看點 |

---

## 🎯 閱讀順序

```
第一次使用 → [QUICK_START.md] ← 快速燒錄、Debug、驗證
需要原理   → [01_PROBLEM.md]  ← LIBUSB / CMSIS-DAP / NULINK Protocol
需要設定   → [02_SOLUTION.md] ← OpenOCD 設定檔 + GDB 指令
VSCode     → [03_VSCODE.md]   ← launch.json 設定
驗證       → [04_VERIFICATION.md] ← 預期輸出 + Debug 等級矩陣
實測驗證   → [05_GDB_DEBUG_TEST.md] ← GDB 指令實測結果（最新）
```

---

## 🚀 快速鏈接

**🚀 立即燒錄 + Debug：**
- [快速燒錄（CLI）](QUICK_START.md#1-燒錄韌體)
- [VSCode F5 Debug](QUICK_START.md#2-debugvscode)
- [驗證 ICE 連線](QUICK_START.md#3-驗證-ice-連線)

**🧪 GDB Debug 實測（2026-03-30 新增）：**
- [GDB 功能驗證結果](05_GDB_DEBUG_TEST.md) — 單步、斷點、Free Run、記憶體、Watchpoint 全部通過 ✅
- [GDB CLI 指令表](QUICK_START.md#4-gdb-cli-debug指令列模式)

**🔍 問題排查：**
- [LIBUSB_ERROR_ACCESS](01_PROBLEM.md#libusb_error_access)
- [CMSIS-DAP vs NULINK Protocol](01_PROBLEM.md#cmsis-dap-vs-nulink)
- [OpenOCD Build 比較](01_PROBLEM.md#三個-openocd-build)
- [常見問題](QUICK_START.md#️-常見問題)

---

## 📌 關鍵發現摘要

### ✅ 已解決：第一代 Nu-Link + OpenOCD + GDB Debug

| 項目 | 值 |
|------|---|
| **OpenOCD Build** | `tool\OpenOCD\bin\openocd.exe` (0.12.0+dev 2026-03-25) |
| **Driver** | `hla` (HLA, not CMSIS-DAP) |
| **Protocol** | Proprietary HID（第一代 Nu-Link 非標準 CMSIS-DAP）|
| **M487 IDCODE** | `0x2BA01477` |
| **Speed** | 4MHz SWD |
| **Breakpoints** | 6 個（硬體）|
| **Watchpoints** | 4 個（硬體）|

### GDB Debug 功能驗證（2026-03-30）

| 功能 | 狀態 |
|------|------|
| ELF Load + Flash | ✅ 63KB @ 31MB/s |
| Memory Read/Write | ✅ |
| Register Read | ✅ |
| Breakpoint | ✅ |
| Single Step | ✅ |
| Free Run + Re-attach | ✅ |
| Watchpoint | ✅ |
| Disassembly | ✅ |

---

## 📁 目錄結構

```
doc\
└── ICE\
    ├── 00_INDEX.md              ← 本文件
    ├── QUICK_START.md           ← 🚀 快速上手（新手起點）
    ├── 01_PROBLEM.md            ← 問題分析
    ├── 02_SOLUTION.md           ← 解決方案
    ├── 03_VSCODE.md             ← VSCode Debug
    ├── 04_VERIFICATION.md       ← 驗證方法
    └── 05_GDB_DEBUG_TEST.md    ← 🎯 GDB 實戰驗證報告
```

---

## 🔗 相關文檔

- [VSCode OpenOCD GDB IDE Setup](../IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md)
- [Nu-Link 使用經驗彙整](../NuLink/NuLink_Experience_Compilation.md)
- [KeroroTeam 共享情報](../../KeroroTeam/knowledge/hardware/ICE-M487/00_OVERVIEW.md)

---

*最後更新：2026-03-30 16:44*