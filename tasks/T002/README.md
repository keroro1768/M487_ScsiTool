# T002 — arm-none-eabi-gcc 編譯環境

**狀態：** ✅ 完成  
**起始日期：** 2026-03-26

## 工具鏈

- **Compiler:** xpack-arm-none-eabi-gcc 15.2.1-1.1
- **Location:** `C:\Users\rinry\Tool\xpack-arm-none-eabi-gcc-15.2.1-1.1\`
- **Make:** mingw32-make（via CMake/mingw64）

## 編譯方法

### VENDOR_LBK（驗證用）

```powershell
cd D:\AiWorkSpace\KM\M487-Examples\Projects\HSUSBD_VENDOR_LBK\build_gcc
mingw32-make -f Makefile all
```

### Composite 韌體

```powershell
cd D:\AiWorkSpace\M487_ScsiTool\firmware\composite\build_gcc
mingw32-make -f Makefile all
```

## Makefile 結構

每個專案有獨立的 Makefile：
- 定義 `CC`, `OBJCOPY`, `SIZE` 路徑
- 定義 BSP 路徑 `BSP_DIR`
- 列出所有 C 來源檔
- 使用 pattern rules 編譯

## 包含的驅動

| 驅動 | 說明 |
|------|------|
| `hsusbd.c` | USB HS Device |
| `usbd.c` | USB Device core |
| `usci_i2c.c` | USCI I2C 控制器 |
| `clk.c` | 時鐘控制 |
| `gpio.c` | GPIO |
| `sys.c` | 系統控制 |
| `pdma.c` | PDMA |
| `fmc.c` | Flash memory controller |
| `uart.c` | UART（Debug port）|
| `retarget.c` | printf 重導向 |
| `system_M480.c` | System init |

## 燒錄

參考 `T003/`
