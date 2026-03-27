# T003 — 燒錄流程建立

**狀態：** ✅ 完成  
**起始日期：** 2026-03-26

## 工具

- **OpenOCD:** Nuvoton OpenOCD 專用版
  - Location: `C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe`
- **燒錄器:** Nu-Link（透過 USB）
  - VID=0x0416, PID=0x511c

## 燒錄指令

```bat
"C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" ^
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" ^
  -f interface/nulink.cfg ^
  -f target/numicroM4.cfg ^
  -c "init" ^
  -c "reset halt" ^
  -c "flash write_image erase <BIN_PATH> 0" ^
  -c "reset run" ^
  -c "shutdown"
```

## 已驗證的燒錄速度

| 韌體 | 大小 | 時間 | 速度 |
|------|------|------|------|
| VENDOR_LBK | 43KB | 3.28s | ~13 KiB/s |
| Composite | 49KB | 7.12s | ~6.7 KiB/s |

## Flash Script 位置

- `D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_VENDOR_LBK\build_gcc\flash.bat`
- `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build_gcc\flash.bat`

## 注意

- 燒錄時需將 Nu-Link USB 接到電腦
- M487 需保持供電
- 燒錄完成後韌體會自動執行（reset run）
