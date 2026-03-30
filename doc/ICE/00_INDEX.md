# M487 ICE 連線系統文檔索引

> 整理時間：2026-03-30  
> 狀態：✅ 已完全解決

---

## 📋 文檔總覽

| 文件 | 說明 |
|------|------|
| **[01_PROBLEM.md](01_PROBLEM.md)** | 問題分析：LIBUSB_ERROR_ACCESS 根本原因 |
| **[02_SOLUTION.md](03_SOLUTION.md)** | 解決方案：成功配置與快速啟動指南 |
| **[03_CONFIG.md](03_SOLUTION.md)** | 詳細設定：各類設定檔解析 |
| **[04_VERIFICATION.md](04_VERIFICATION.md)** | 驗證方法：連線測試與功能驗證 |
| **[05_VSCODE.md](05_VSCODE.md)** | VSCode Debug 整合：launch.json 設定 |

---

## 🎯 快速鏈接

**🚀 立即使用：**
- [快速啟動指令](03_SOLUTION.md#快速啟動)
- [nulink_m487_ice.cfg](03_SOLUTION.md#nulink_m487_icecfg)
- [VSCode F5 Debug](05_VSCODE.md)

**🔍 問題排查：**
- [LIBUSB_ERROR_ACCESS 原因](01_PROBLEM.md#libusb_error_access)
- [CMSIS-DAP vs NULINK Protocol](01_PROBLEM.md#cmsis-dap-vs-nulink)
- [OpenOCD Build 比較](01_PROBLEM.md#三個-openocd-build)

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
| `M487_ScsiTool\tool\openocd-build\bin\openocd.exe` (單獨) | 缺少 MSYS2 DLL |

---

## 📁 目錄結構

```
D:\AiWorkSpace\M487_ScsiTool\
└── doc\
    └── ICE\
        ├── 00_INDEX.md          ← 本文件
        ├── 01_PROBLEM.md         ← 問題分析
        ├── 02_SOLUTION.md        ← 解決方案（中文）
        ├── 03_CONFIG.md          ← 詳細設定檔
        ├── 04_VERIFICATION.md    ← 驗證方法
        └── 05_VSCODE.md          ← VSCode 整合
```

---

## 🔗 相關文檔

- [VSCode OpenOCD GDB IDE Setup](../IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md)
- [Nu-Link 使用經驗彙整](../NuLink/NuLink_Experience_Compilation.md)
- [KeroroTeam 共享情報](../../KeroroTeam/knowledge/hardware/ICE-M487/00_OVERVIEW.md)

---

*最後更新：2026-03-30 11:30*
