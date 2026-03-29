# T012 VERIFY — Toolchain Review Report

**Task:** T012 — T002/T003 工具鏈 Review  
**Date:** 2026-03-28  
**Reviewer:** Giroro (Subagent)  
**Status:** in-progress → issues found

---

## 1. Makefile 結構一致性審視

### 1.1 Composite + HID-over-I2C 兩專案比較

| 項目 | `firmware/composite/build_gcc/Makefile` | `firmware/hid-over-i2c/Makefile` |
|------|------------------------------------------|----------------------------------|
| 工具鏈預設值 (`?=`) | ✅ 一致 | ✅ 一致 |
| `XPKG_ROOT` | `C:/Users/rinry/Tool/xpack-arm-none-eabi-gcc-15.2.1-1.1` | 同 |
| `OPENOCD_ROOT` | `C:/Users/rinry/Tool/OpenOCD-Nuvoton` | 同 |
| `BSP_DIR` | `D:/AiWorkSpace/KM/M480BSP` | 同 |
| Build 目錄 | `build_gcc` (相對 Makefile 位置) | `build` (相對 Makefile 位置) |
| 來源結構 | `composite/*.c` + BSP | `hid-over-i2c/src/*.c` + BSP |
| `startup_M480.S` | ✅ 存在 | ✅ 存在 |
| `gcc_arm.ld` | ✅ 存在 | ✅ 存在 |
| `msc_debug.c` | ✅ 存在 | N/A (非 MSC 專案) |
| `uart_debug.c` | ✅ 存在 | N/A |

### 1.2 路徑相依性檢查

| 路徑 | Makefile 預設值 | 驗證 |
|------|----------------|------|
| `BSP_DIR` → `D:/AiWorkSpace/KM/M480BSP` | ✅ 硬編碼預設值 | ⚠️ 需手動建立 `Makefile.config` 才能跨機器 |
| `XPKG_ROOT` → `C:/Users/rinry/Tool/xpack-arm-none-eabi-gcc-15.2.1-1.1` | ✅ 硬編碼預設值 | ⚠️ 同上 |
| OpenOCD scripts (`nulink.cfg`, `numicroM4.cfg`) | ✅ 存在於 `OPENOCD_ROOT/scripts/` | ✅ 驗證通過 |

**Build 目錄自動建立：** ✅ `$(BUILD_DIR): cmd /c "if not exist..."` 語法正確  
**clean 目標：** ✅ 使用 `cmd /c del` 配合 `|| exit 0` 防止錯誤退出

### 1.3 問題彙整

| 嚴重度 | 問題 | 說明 |
|--------|------|------|
| ⚠️ Medium | `BSP_DIR` 預設為絕對路徑 | 跨機器使用需建立 `Makefile.config`，未提供提示 |
| ⚠️ Low | `make flash` 目標缺少驗證 | `flash.bat` 有 VERIFY 模式，但 `make flash` 無 |

---

## 2. flash.bat 脚本跨專案共用性

### 2.1 路徑環境變數使用情况

| 檢查項目 | 狀態 | 說明 |
|----------|------|------|
| `OPENOCD_ROOT` 環境變數 | ✅ | 有預設值，且可被 `Makefile.config` 設定 |
| `%~dp0` 用於 script 自身目錄 | ✅ | `FIRMWARE_BIN=%SCRIPT_DIR%firmware.bin` 正確 |
| 可外部覆寫 `FIRMWARE_BIN` | ❌ | 硬編碼為 `%~dp0firmware.bin`，無法透過環境變數指定 |
| OpenOCD 路徑驗證 | ✅ | 三段式檢查（bin、scripts、firmware） |
| 錯誤訊息清楚 | ✅ | 每個錯誤都有具體說明 |

### 2.2 錯誤處理

| 檢查項目 | 狀態 | 說明 |
|----------|------|------|
| OpenOCD binary 不存在 → exit 1 | ✅ | |
| firmware.bin 不存在 → exit 2 | ✅ | |
| OpenOCD scripts 不存在 → exit 3 | ✅ | |
| OpenOCD 執行失敗 → exit code 傳遞 | ✅ | `set "EXIT_CODE=%errorlevel%"` 然後檢查 |
| 成功訊息 | ✅ | `[SUCCESS] Flash completed successfully.` |

### 2.3 問題彙整

| 嚴重度 | 問題 | 說明 |
|--------|------|------|
| ⚠️ Medium | `FIRMWARE_BIN` 無法外部覆寫 | 若要燒錄其他路徑的 firmware，需修改 script |
| ✅ Low | `VERIFY` 模式正確使用 `verify_image` | 燒錄後驗證功能正確實作 |

---

## 3. OpenOCD Script 完整性

### 3.1 `m487_cmsis_dap.cfg` 審視

```tcl
source D:/AiWorkSpace/M487_ScsiTool/tool/openocd/m487_target.cfg
```

| 檢查項目 | 狀態 | 說明 |
|----------|------|------|
| VID/PID (`0x0416 0x511C`) | ✅ | 對應 Nuvoton Nu-Link |
| `transport select swd` | ✅ | M487 支援 SWD |
| 呼叫 `m487_target.cfg` | ✅ | 目標晶片設定正確 |
| `cmsis-dap` driver 宣告 | ✅ | |

**⚠️ 重大問題：**

- **絕對路徑** `source D:/AiWorkSpace/M487_ScsiTool/tool/openocd/m487_target.cfg`  
  → 若專案移到其他位置或在其他機器上，會斷開  
  → 應改用相對路徑：`source [find tool/openocd/m487_target.cfg]` 或 `../tool/openocd/m487_target.cfg`

- **整合狀態不明**：此 config 目前**未被 `flash.bat` 或 `Makefile` 使用**，僅用於 CMSIS-DAP 燒錄情境，需確認是否為預期用途

### 3.2 `m487_target.cfg` 審視

| 檢查項目 | 狀態 | 說明 |
|----------|------|------|
| CHIPNAME 預設 `M487` | ✅ | |
| `CPUDAPID 0x2BA01477` | ✅ | M487 SWD DP-ID 正確 |
| `WORKAREASIZE 0x20000` (128KB) | ✅ | M487 SRAM 128KB |
| OpenOCD 0.12.x DAP 語法 | ✅ | `swd newdap`, `dap create`, `target create` |
| `adapter speed 4000` | ✅ | 合理預設值 (4MHz) |
| `reset_config none` + `cdb_reset_config sysresetreq` | ✅ | 相容於 M4 soft reset |
| `work-area-backup 0` | ✅ | 不備份 work-area 內容 |
| `gdb-detach` event | ✅ | debugger 連線中斷時正確 shutdown |

**Flash Banks 對照：**

| Bank 名稱 | 位址 | 大小 | 狀態 |
|-----------|------|------|------|
| `flash_aprom` | `0x00000000` | — | ✅ M487 APROM (512KB) |
| `flash_ldrom` | `0x00100000` | — | ✅ M487 LDROM |
| `flash_sprom` | `0x00200000` | — | ✅ M487 SPROM |
| `flash_config` | `0x00300000` | — | ✅ M487 CONFIG |
| `flash_dfmc_data` | `0x00400000` | — | ✅ DFMC data |

> ⚠️ 注意：`flash_data` bank 設在 `0x0001F000` 為 M487 Data Flash (2KB)，需確認此位址是否與 actual flash layout 匹配。

### 3.3 `nulink.cfg` + `numicroM4.cfg`（flash.bat 實際使用）

| Config | 狀態 | 說明 |
|--------|------|------|
| `interface/nulink.cfg` | ✅ 存在 | HLA interface, `cmsis_dap_vid_pid` 一致 |
| `target/numicroM4.cfg` | ✅ 存在 | 對應 `numicroM4` 系列，包含 M487 |

---

## 4. 發現的關鍵不一致

| 項目 | `m487_cmsis_dap.cfg` 流程 | `flash.bat` / `make flash` 流程 |
|------|--------------------------|--------------------------------|
| Interface config | `interface/cmsis-dap.cfg` | `interface/nulink.cfg` |
| Target config | `m487_target.cfg` (自訂) | `target/numicroM4.cfg` (OpenOCD 內建) |
| 驗證燒錄 | ❌ 無 `verify_image` | ✅ `verify_image` (VERIFY=1 時) |
| 整合狀態 | ⚠️ 未被任何入口使用 | ✅ 為主要燒錄方式 |

---

## 5. 總結與建議

### ✅ 正常項目
- 兩 Makefile 結構高度一致，工具鏈設定可重現
- `flash.bat` 錯誤處理完整，環境變數使用合理
- `m487_target.cfg` 符合 OpenOCD 0.12.x 語法，DAP ID / Flash banks 設定正確
- OpenOCD 工具鏈 (`nulink.cfg` + `numicroM4.cfg`) 完整且可正常運作

### ⚠️ 需修正項目（依優先順序）

1. **[High] `m487_cmsis_dap.cfg` 絕對路徑問題**  
   將 `source D:/AiWorkSpace/M487_ScsiTool/tool/openocd/m487_target.cfg`  
   改為 `source [find tool/openocd/m487_target.cfg]`

2. **[Medium] `flash.bat` 的 `FIRMWARE_BIN` 無法外部覆寫**  
   建議加入：`if defined FIRMWARE_BIN (set "FIRMWARE_BIN=%FIRMWARE_BIN%") else (...)`

3. **[Medium] `make flash` 缺少燒錄驗證**  
   建議比照 `flash.bat` 加入 `verify_image` 步驟

4. **[Low] `m487_cmsis_dap.cfg` 整合狀態不明**  
   建議在 `m487_cmsis_dap.cfg` 頂部加上註解說明此 config 的使用情境

---

## 6. Review 結論

**編譯環境（Makefile）：** ✅ 可用，結構一致  
**燒錄流程（flash.bat）：** ✅ 基本正確，有小幅改進空間  
**OpenOCD Scripts（m487_target.cfg）：** ✅ 正確  
**OpenOCD Scripts（m487_cmsis_dap.cfg）：** ⚠️ 有絕對路徑問題需修正

> 建議優先修正 `m487_cmsis_dap.cfg` 的絕對路徑，使其具備跨機器可攜性。其餘項目影響較小，可在後續迭代中處理。
