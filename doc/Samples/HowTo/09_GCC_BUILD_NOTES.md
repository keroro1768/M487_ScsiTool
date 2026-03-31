# 09 — GCC 編譯範例程式注意事項

> 建立：2026-03-31
> 適用：M487 BSP Sample Code（HSUSBD 系列）使用 xpack arm-none-eabi-gcc 編譯

---

## 重大問題：BSP 預設 Linker Script 指向 LDROM

### 症狀

使用 GCC 編譯 BSP 範例（如 `HSUSBD_HID_Transfer`）後燒錄至 M487，USB 裝置無法被 Windows 辨識，Device Manager 中完全看不到裝置。

### 根因

BSP 預設的 GCC linker script 位於：
```
M480BSP/Library/Device/Nuvoton/M480/Source/GCC/gcc_arm.ld
```

其 FLASH 起始位址為 **LDROM**：
```ld
FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 0x80000   /* LDROM! */
```

但 OpenOCD `flash write_image` 預設燒錄至 **APROM（0x00000000）**，導致韌體的向量表、中斷處理函式位址全部錯誤。CPU 從 APROM 開機後跳轉到錯誤位址，韌體無法正常運行。

### 驗證方式

檢查 `.map` 檔中 `USBD20_IRQHandler` 的位址：
```
# 錯誤（LDROM）：
0x10000770    USBD20_IRQHandler

# 正確（APROM）：
0x00000770    USBD20_IRQHandler
```

### 解法

使用修正過的 linker script，將 FLASH ORIGIN 改為 `0x00000000`：

```ld
FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x80000   /* 512K APROM */
RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 0x20000   /* 128K */
```

已驗證可用的 linker script 位於：
```
doc/Samples/Projects/Projects/HSUSBD_Mass_Storage_ShortPacket/Library/Device/Nuvoton/M480/Source/GCC/gcc_arm.ld
```

或使用本專案的自訂版本：
```
firmware/composite/gcc_arm_aprom.ld
```

---

## GCC 編譯環境設定

### 工具鏈

| 工具 | 路徑 |
|------|------|
| arm-none-eabi-gcc | `C:/Users/rinry/Tool/xpack-arm-none-eabi-gcc-15.2.1-1.1/bin/` |
| OpenOCD (Nuvoton) | `C:/Users/rinry/Tool/OpenOCD-Nuvoton/OpenOCD/bin/openocd.exe` |

### 編譯參數

```
-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
-O2 -Wall -fdata-sections -ffunction-sections -fno-strict-aliasing -fno-builtin
```

### 連結參數

```
-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
-T<linker_script> -Wl,--gc-sections --specs=nosys.specs --specs=nano.specs
```

### TEMP 目錄問題

在 Git Bash / MSYS2 bash 中直接呼叫 `make` 可能遇到：
```
Cannot create temporary file in C:\WINDOWS\: Permission denied
```
**解法：** 使用 `cmd.exe //c build.bat` 執行編譯，讓 GCC 正確繼承 Windows 環境變數。

---

## 已驗證的範例程式

### HSUSBD_Mass_Storage_ShortPacket（MSC）

| 項目 | 內容 |
|------|------|
| 來源 | `doc/Samples/Projects/Projects/HSUSBD_Mass_Storage_ShortPacket/` |
| VID/PID | 0x0416 / 0x0470 |
| Build 腳本 | `GCC/build.bat` |
| 大小 | text=13,540 data=480 bss=692（約 14.7KB）|
| 結果 | Windows Device Manager 顯示 USB Mass Storage |

所需 BSP 源碼：`system_M480.c`, `hsusbd.c`, `clk.c`, `sys.c`, `uart.c`, `retarget.c`, `startup_M480.S`

### HSUSBD_HID_Transfer（HID）

| 項目 | 內容 |
|------|------|
| 來源 | `doc/Samples/USB_HS_Samples/HSUSBD_HID_Transfer/` |
| VID/PID | 0x0416 / 0x5020 |
| Build 腳本 | `GCC/build.bat` |
| 大小 | text=12,340 data=536 bss=1,640（約 14.5KB）|
| 結果 | Windows Device Manager 顯示 HID Device（需修正 linker script）|

所需 BSP 源碼：同上

---

## 燒錄命令

```bash
openocd -s "<OPENOCD_ROOT>/scripts" \
  -f interface/nulink.cfg -f target/numicroM4.cfg \
  -c "init" -c "reset halt" \
  -c "flash write_image erase <firmware.bin> 0" \
  -c "reset run" -c "shutdown"
```

燒錄後需**拔插 USB Device 纜線**讓 Host 重新列舉。

---

## Checklist：新增 GCC 編譯範例時

1. **Linker script** — 確認 `FLASH ORIGIN = 0x00000000`（APROM），不要使用 BSP 預設的 `0x10000000`
2. **Build 方式** — 使用 `cmd.exe //c build.bat`，避免 bash 下 TEMP 路徑問題
3. **Include 路徑** — 需包含 `Library/StdDriver/inc`（BSP 的 `M480.h` 會 `#include "sys.h"` 等）
4. **驗證 .map** — 檢查 `.text` 起始位址應為 `0x00000000`，非 `0x10000000`
5. **USB 重新列舉** — 燒錄後拔插 USB Device 纜線

---

*最後更新：2026-03-31*
