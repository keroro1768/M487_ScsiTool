# E001 - M487 ICE 連線突破 + OpenOCD Debug 整備

> **重大發現日期：** 2026-03-30  
> **發現者：** Giroro  
> **類型：** 🔥 重大突破（硬體瓶頸解除）

---

## 🎯 核心發現

**第一代 Nu-Link (VID=0x0416, PID=0x511C) + OpenOCD 連線問題已完全解決**

| 項目 | 之前（錯誤）| 之後（正確）|
|------|-------------|-------------|
| OpenOCD binary | `openocd_cmsis-dap.exe` | `openocd-build\bin\openocd.exe` |
| OpenOCD driver | `cmsis-dap` | `hla` + `hla layout nulink` |
| 設定檔 | `m487_cmsis_dap.cfg` | `nulink_m487_ice.cfg` |
| 燒錄 | 待硬體 | ✅ 立即可執行 |
| VSCode Debug F5 | 待硬體 | ✅ 立即可執行 |

---

## 📊 對現有任務的影響

| 任務 | 舊狀態 | 新狀態 | 說明 |
|------|--------|--------|------|
| **T001** | ⏳ 待硬體 | 🔄 可燒錄/Debug | OpenOCD ICE 已驗證可用 |
| **T003** | ✅ Finish | ✅ Finish（內容更新）| 燒錄路徑/命令已更新 |
| **T005** | ✅ Finish（未優化）| ✅ Finish | VSCode Debug 已驗證可行 |
| **T010** | ⏳ 待硬體 | 🔄 可執行 | 可直接 Debug 而非等實體測試 |
| **T033** | ✅ Framework | ✅ 可執行 | OpenOCD GDB 驗證框架已就緒 |
| **T034** | ✅ Framework | ✅ 可執行 | DWT 可透過 OpenOCD 驗證 |

---

## 📁 已更新的文件

| 檔案 | 動作 |
|------|------|
| `doc/ICE/00_INDEX.md` | ✅ 新建 |
| `doc/ICE/01_PROBLEM.md` | ✅ 新建 |
| `doc/ICE/02_SOLUTION.md` | ✅ 新建 |
| `doc/ICE/03_VSCODE.md` | ✅ 新建 |
| `doc/ICE/04_VERIFICATION.md` | ✅ 新建 |
| `doc/ICE/QUICK_START.md` | ✅ 新建（快速上手指南）|
| `tool/openocd/nulink_m487_ice.cfg` | ✅ 新建（驗證過的設定檔）|
| `tool/openocd/openocd.bat` | ✅ 新建（wrapper，含 MSYS2 DLL）|
| `firmware/composite/.vscode/launch.json` | ✅ 更新 |
| `doc/NuLink/NuLink_Experience_Compilation.md` | ✅ 修正 |
| `doc/IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md` | ✅ 大幅修正 |
| `doc/hardware/M487/02_TOOLS.md` | ✅ 更新 |
| `doc/hardware/M487/03_FLASH_READ_WRITE.md` | ✅ 更新 |
| `doc/hardware/M487/05_OPENOCD_REF.md` | ✅ 更新 |
| `doc/hardware/M487/06_HSUSBD_COMPILE_FLASH.md` | ✅ 更新 |

---

## 🚀 下一步優化建議

### 立即可做（不需要硬體）

1. **T001 燒錄驗證** — 使用 `openocd.bat` 燒錄 `firmware.bin`，確認 USB MSC + HID 枚舉
2. **T010 Debug 實作** — 在 VSCode 中設定斷點，單步執行驗證邏輯
3. **T027 USB Filter Driver** — WDK 安裝後可繼續（T027a WDK 安裝）
4. **T006 Windows C++ HID Tool** — 可開始研究實作

### 待確認（需要 M487 實體）

1. **USB VID=0x04F3 PID=0x0732 枚舉** — 確認複合裝置正確識別
2. **MSC RAM Disk 功能** — 電腦看到 USB Drive
3. **HID I2C 通訊** — 用 hidtool 發送 HID I2C 命令
4. **ITM SWO trace** — 驗證 T024 的 ITM 輸出

---

## ✅ 驗證狀態（已實際測試）

```
Info : Nu-Link firmware_version 7946, product_id (0x40012009)
Info : Adapter is Nu-Link
Info : IDCODE: 0x2BA01477
Info : [M487.cpu] Cortex-M4 r0p1 processor detected
Info : [M487.cpu] target has 6 breakpoints, 4 watchpoints
[M487.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x100028f0 msp: 0x20020000
```

---

## 📌 待 Karoro 決策

1. **是否將 T001 改為「🔄 進行中」？**（燒錄驗證現在可以執行）
2. **是否指派 T010 VSCode Debug 實作？**（Dororo 驗證）
3. **T006 Windows C++ HID Tool 是否列為下個優先任務？**

---

*E001 - ICE 突破發現 — 2026-03-30*
