# M487 Flash 讀寫流程

> ⚠️ **2026-03-30 更新**：OpenOCD 路徑已更正，請使用 `openocd.bat` + `nulink_m487_ice.cfg`

## 前提條件

1. **Nu-Link 驅動已安裝**
   - Interface 1 應為 `Nuvoton Nu-Link USB` (WinUSB)
   - 檢查: 裝置管理員 → Nuvoton Nu-Link USB

2. **OpenOCD 已就緒**
   - 路徑: `D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat`（推薦）
   - 或: `D:\AiWorkSpace\M487_ScsiTool\tool\openocd-build\bin\openocd.exe`

3. **確認 M487 連接**
   ```
   開機時 Nu-Link 應顯示連線燈
   ```

---

## 快速測試 (連線驗證)

```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat `
  -s "D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg" `
  -c "init" -c "targets" -c "shutdown"
```

**預期輸出:**
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

---

## 燒錄 Flash

> ⚠️ **2026-03-30 更新**：使用 `openocd.bat` + `nulink_m487_ice.cfg`

### 燒錄整個檔案
```powershell
D:\AiWorkSpace\M487_ScsiTool\tool\openocd\openocd.bat `
  -s "D:\AiWorkSpace\M487_ScsiTool\tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f "D:\AiWorkSpace\M487_ScsiTool\tool\openocd\nulink_m487_ice.cfg" `
  -c "init" -c "reset halt" `
  -c "flash write_image erase C:/firmware.bin 0" `
  -c "shutdown"
```

**速度參考:** ~7.4 KiB/s，512KB 約 70 秒

**成功輸出：**
```
Info : Write 63996 bytes to flash
Info : Verified OK
```

### 燒錄後驗證
燒錄時 OpenOCD 會自動校驗寫入的資料

---

## 常用 OpenOCD 指令

| 指令 | 功能 |
|------|------|
| `init` | 初始化 Debug 介面 |
| `reset halt` | 重置並halt CPU |
| `halt` | halt CPU |
| `resume` | 繼續執行 |
| `targets` | 列出所有 target |
| `flash read_bank 0 <file> <offset> <size>` | 讀取 Flash |
| `flash write_image erase <file> <offset>` | 燒錄 Flash |
| `flash erase_address <addr> <size>` | 抹除 Flash |
| `shutdown` | 關閉 OpenOCD |

---

## 燒錄後驗證 (MD5 比對)

```powershell
# 原始檔 MD5
(Get-FileHash "C:\Users\rinry\m487_flash.bin" -Algorithm MD5).Hash

# 燒錄後讀回 MD5
(Get-FileHash "C:\Users\rinry\m487_verify.bin" -Algorithm MD5).Hash
```

兩者應完全一致

---

## 疑難排解

| 問題 | 解決方案 |
|------|----------|
| `LIBUSB_ERROR_ACCESS` | 確認 Nu-Link 驅動為 WinUSB，確認 USB 權限 |
| `Couldn't open file` | 確認輸出路徑存在，確認非admin有寫入權限 |
| 燒錄後無法啟動 | 確認 vector table 正確，確認 SP/PC 值正確 |
| 讀取全是 0xFF | 確認 Flash 未被抹除，或晶片損壞 |
