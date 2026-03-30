# 驗證方法

> 日期：2026-03-30

---

## ✅ 快速驗證清單

### 基本連線測試

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "init" -c "targets" -c "shutdown"
```

**預期輸出：**
```
Info : clock speed 4000 kHz
Info : Nu-Link firmware_version 7946, product_id (0x40012009)
Info : Adapter is Nu-Link
Info : IDCODE: 0x2BA01477
Info : [M487.cpu] Cortex-M4 r0p1 processor detected
Info : [M487.cpu] target has 6 breakpoints, 4 watchpoints
    TargetName         Type       Endian TapName            State
--  ------------------ ---------- ------ ------------------ ------------
 0* M487.cpu           hla_target little M487.cpu           unknown
shutdown command invoked
```

### Reset Halt 測試

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "init" -c "reset halt" -c "targets" -c "shutdown"
```

**預期輸出：**
```
[M487.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x01000000 pc: 0x100028f0 msp: 0x20020000
```

### 寄存器讀取測試

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat -c "init" -c "reset halt" -c "reg pc" -c "reg xpsr" -c "shutdown"
```

**預期輸出：**
```
M487.cpu halted
xPSR: 0x01000000
pc: 0x100028f0
```

### 記憶體讀取測試

```tcl
mdw 0x20000000 10
```

**預期輸出：** 顯示 SRAM 內容

---

## 🔍 Debug 等級驗證矩陣

| 等級 | 功能 | 工具 | 驗證指令 |
|------|------|------|---------|
| L1 | UART Log | 序列埠 | 115200 8N1 |
| L2 | MSC Debug Channel | `msc_debug.exe` | `cdb_read_ID` |
| L2 | hidtool | Python | `hidtool.py info` |
| L3 | OpenOCD + ICE | `openocd.bat` | `reset halt` ✅ |
| L4 | GDB Debug | VSCode F5 | breakpoints ✅ |

---

## 📊 預期輸出對照

### 成功輸出

```
Info : Nu-Link firmware_version 7946, product_id (0x40012009)  ← 晶片正確識別
Info : Adapter is Nu-Link                                    ← NULINK protocol ✅
Info : IDCODE: 0x2BA01477                                    ← M487 確認 ✅
Info : [M487.cpu] Cortex-M4 r0p1 processor detected          ← Core 確認 ✅
Info : [M487.cpu] target has 6 breakpoints, 4 watchpoints    ← Debug 資源確認 ✅
```

### 常見失敗輸出

| 輸出 | 原因 | 解決 |
|------|------|------|
| `LIBUSB_ERROR_ACCESS` | DLL 缺失或權限不足 | 設定 MSYS2 PATH |
| `No adapter layout 'nulink'` | 使用錯誤的 OpenOCD build | 換用 openocd-build |
| `CMSIS-DAP command CMD_INFO failed` | 用了錯誤的 driver | 用 `hla` 不是 `cmsis-dap` |
| `BUG: current_target out of bounds` | 沒有建立 target | 確認 nulink_m487_ice.cfg 正確 |
| `invalid command name "swd newdap"` | 語法錯誤 | 確認 OpenOCD 0.12 語法 |

---

*最後更新：2026-03-30 11:30*
