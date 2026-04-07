# T001-ST1 研究報告：HSUSBD_Mass_Storage_ShortPacket KEIL → GCC 移植可行性評估

**任務代號：** T001-ST1  
**日期：** 2026-03-30  
**研究性質：** 純研究（不燒錄硬體）

---

## 1. KEIL 專案燒錄流程分析

### 1.1 目標裝置與 Flash 演算法

| 參數 | 數值 |
|------|------|
| **Device** | `M487JIDAE` |
| **Flash Driver** | `M481_AP_512.FLM`（M481 512KB APROM 演算法） |
| **Flash Base** | `0x00000000` |
| **Flash Size** | `0x080000`（512KB）|
| **RAM for Algorithm** | `0x20000000` |
| **RAM Size for Algorithm** | `0x4000`（16KB）|
| **Compiler** | ARMCLANG V6.24（uAC6=1）|
| **Clock** | 84MHz |

**⚠️ 重要發現：** 此 Sample 的 KEIL 專案使用 **M481** 的 Flash 演算法（M481_AP_512.FLM），而非 M487 的演算法。M481 有 512KB Flash，但 M487 只有 256KB Flash。這意味著：
- 燒錄時實際上使用 512KB Flash 演算法（可能是 KEIL Pack 中未提供 M487 版本）
- GCC 移植時，需確認 M487 實際 Flash 大小，選擇正確的 linker script（`gcc_arm.ld` 定義 512KB，`gcc_arm_160k.ld` 可能更接近 M487 實際大小）
- **待確認：** M487JIDAE 的 Flash 大小（可能為 256KB 或 512KB）

### 1.2 Nu_Link_Driver.ini 燒錄設定

```
FlashDriverName=M481_AP_512.FLM
FlashBase=0x00000000
FlashSelect=APROM
RAMForAlgorithmStart=0x20000000
RAMForAlgorithmSize=0x4000
ResetAndRun=0        ← 燒錄後不自動執行
EnableFlashBreakpoint=1
```

### 1.3 KEIL 燒錄流程

1. KEIL 呼叫 `Nu_Link.dll`（Flash2）
2. `Nu_Link.dll` 將 Flash Algorithm（FLM）下載到晶片 RAM（0x20000000）
3. 燒錄資料透過 RAM 中的演算法寫入 Flash
4. `ResetAndRun=0`：燒錄後需要手動 reset 或從 debugger 執行
5. Debug 連接使用 `Bin\Nu_Link.dll`，Target DLL 選擇 `UL2CM3`

### 1.4 KEIL 專案原始碼檔案

| 群組 | 檔案 |
|------|------|
| **CMSIS** | `system_M480.c`，`startup_M480.s`（ARM assembly）|
| **User** | `main.c`，`descriptors.c`，`MassStorage.c` |
| **Library** | `hsusbd.c`，`clk.c`，`sys.c`，`uart.c`，`retarget.c` |

---

## 2. BSP Library 結構分析

### 2.1 目錄位置

```
D:\AiWorkSpace\KM\M480BSP\
├── Library\
│   ├── Device\Nuvoton\M480\
│   │   ├── Include\          ← 登錄檔、CMSIS 頭檔（NuMicro.h, system_M480.h...）
│   │   └── Source\
│   │       ├── ARM\           ← KEIL/ADS startup（startup_M480.s）
│   │       └── GCC\           ← GCC startup + linker scripts
│   └── StdDriver\
│       └── src\               ← hsusbd.c, clk.c, sys.c, uart.c, retarget.c 等
└── SampleCode\StdDriver\HSUSBD_Mass_Storage_ShortPacket\
    ├── main.c, MassStorage.c, descriptors.c
    ├── KEIL\                  ← KEIL 專案
    ├── GCC\                   ← ⚠️ 只有 .project/.cproject，無 Makefile
    ├── IAR\                   ← IAR 專案
    └── VSCode\               ← VSCode + CMSIS-RTS 配置
```

### 2.2 GCC 支援現況

**✅ GCC Startup Code 存在：**
- `D:\AiWorkSpace\KM\M480BSP\Library\Device\Nuvoton\M480\Source\GCC\startup_M480.S`
- 格式：ARM GNU unified assembly（`.S` 大寫，含 `.syntax unified`）
- 涵蓋完整向量表（含 USBD20_IRQHandler 代號 65，為 HSUSBD 中斷）
- Stack: 0x800, Heap: 0x100
- Reset_Handler 包含 SPIM cache 初始化（`ENABLE_SPIM_CACHE` 巨集開關）
- 呼叫 `SystemInit` → `_start`

**✅ GCC Linker Scripts 存在：**
| 檔案 | Flash 大小 | 用途 |
|------|-----------|------|
| `gcc_arm.ld` | 512KB (0x80000) | M481 |
| `gcc_arm_160k.ld` | 160KB | M487（？）|
| `gcc_arm_64k.ld` | 64KB | |

**⚠️ 待確認：** M487JIDAE 實際 Flash 大小。若為 256KB，需以 `gcc_arm.ld` 為基礎修改。

**✅ GCC 相關附屬檔案：**
- `semihosting.h` — 半主機支援
- `_syscalls.c` — Newlib syscalls 實作（已實作 `_read`, `_write`, `lseek`, `_close` 等）

### 2.3 Library 組織（重要：StdDriver 是預編譯 .o）

StdDriver 中的 `hsusbd.c`、`clk.c` 等是以**預編譯 .o 檔**形式存在，還是 source？從 KEIL 專案 `.uvoptx` 可看出，這些是原始碼 `.c` 檔案（GroupNumber=3, FileType=1），**非預編譯**。因此 GCC 編譯時需要這些原始碼檔。

---

## 3. GCC 編譯可行性評估

### 3.1 GCC 目錄現況

```
HSUSBD_Mass_Storage_ShortPacket\GCC\
├── .project           ← Eclipse 專案描述（空殼）
├── .cproject          ← Eclipse CDT 專案（空殼）
└── preferences.ini    ← Eclipse 喜好設定
```

**❌ GCC 目錄沒有 Makefile。** 這是完整移植的最大工作量起點。

### 3.2 M487_ScsiTool 的 build_gcc/Makefile 參考

**❌ 路徑 `D:\AiWorkSpace\M487_ScsiTool\build_gcc\Makefile` 不存在。**

`D:\AiWorkSpace\M487_ScsiTool\` 下的實際目錄結構：
- `build/`（CMake 輸出）
- `cmake/`（CMakeLists.txt）
- `firmware/`
- `src/`, `include/`, `hid_bridge/` 等

**➡️ 無法直接參考此路徑的 Makefile。**

### 3.3 VSCode tasks.json 分析

VSCode 的 `.vscode/tasks.json` 定義了四個任務：
- **CMSIS Erase**：`pyocd erase --chip`
- **CMSIS Load**：`pyocd load`
- **CMSIS Run**：`pyocd gdbserver`（launch + OpenOCD）
- **CMSIS Load+Run**

**注意：** 這些是 CMSIS-RTS (CSolution) 的工具鏈，**不是**手寫的 GCC Makefile。VSCode 方案依賴：
1. CMSIS Build System（YAML 配方）
2. `pyocd` 燒錄（使用 CMSIS-DAP）
3. `openocd` + `cortex-debug-nuvoton` 擴展除錯

**➡️ VSCode 方案與傳統 GCC Makefile 是不同的開發流程。**

### 3.4 GCC 編譯所需關鍵設定

要建立 GCC Makefile，需要：

| 項目 | 設定值 |
|------|--------|
| **CPU** | `cortex-m4` |
| **FPU** | `fpv4-sp-d16`（硬體 FPU）|
| **指令集** | `thumb` |
| **ABI** | `elf`（KEIL 使用 `armclang`，GCC 使用 `gnueabi`）|
| **優化** | `-O0`~`-O3`（KEIL 預設 `-O1`，需確認）|
| **Linker Script** | `gcc_arm.ld`（512KB）或 `gcc_arm_160k.ld` |
| **Startup** | `startup_M480.S` |
| **System Init** | `system_M480.c` |

**⚠️ 警告：KEIL ARMCLANG 和 GCC 的指標大小/struct 填充行為可能不同，需驗證 binary 相容性。**

### 3.5 GCC 編譯評估總結

| 項目 | 狀態 | 說明 |
|------|------|------|
| GCC Startup code | ✅ | `startup_M480.S` 存在 |
| GCC Linker scripts | ✅ | 3 種大小可選 |
| GCC Makefile | ❌ | 需從零建立 |
| StdDriver 原始碼 | ✅ | `hsusbd.c`, `clk.c` 等可編譯 |
| 參考 Makefile | ❌ | `build_gcc/Makefile` 不存在 |
| CMSIS-RTS | ⚠️ | VSCode 有配置但非 GCC Makefile |

**➡️ 移植可行，但需要從零建立 Makefile。建議以 BSP `Library\StdDriver` 的建置方式為參考。**

---

## 4. KEIL 燒錄設定 → OpenOCD 移植分析

### 4.1 現有 OpenOCD Config 分析

現有 `nulink_m487_ice.cfg`：
```
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd
swd newdap M487 cpu -expected-id 0x2BA01477
target create M487.cpu cortex_m -dap M487.dap
adapter speed 4000
```

**✅ SWD DP-ID 確認：** `0x2BA01477` 為 Nuvoton M48x 系列標準 ID，正確。

### 4.2 OpenOCD numicro Flash Driver 分析

`scripts/target/numicroM4.cfg` 定義了 Nuvoton numicro flash banks：

```
flash bank $_FLASHNAME numicro 0x00000000 0 0 0 $_TARGETNAME   ← APROM
flash bank $_FLASHNAME numicro 0x0001F000 0 0 0 $_TARGETNAME   ← DATA
flash bank $_FLASHNAME numicro 0x00100000 0 0 0 $_TARGETNAME   ← LDROM
flash bank $_FLASHNAME numicro 0x00200000 0 0 0 $_TARGETNAME   ← SPROM
flash bank $_FLASHNAME numicro 0x00300000 0 0 0 $_TARGETNAME   ← CONFIG
flash bank $_FLASHNAME numicro 0x00400000 0 0 0 $_TARGETNAME   ← DFMC_DATA
```

### 4.3 KEIL → OpenOCD 燒錄命令對照

| KEIL 設定 | OpenOCD 對應 |
|-----------|-------------|
| Flash Base `0x00000000` | `flash bank aprom numicro 0x00000000` |
| Flash Driver `M481_AP_512.FLM` | OpenOCD numicro driver（無 FLM，用內建驅動）|
| RAM for algo `0x20000000`, `0x4000` | `work-area-phys 0x20000000 -work-area-size 0x4000` |
| ResetAndRun=0 | `reset halt`（而非 `reset run`）|

### 4.4 OpenOCD 燒錄建議命令序列

```tcl
# === 連接與初始化 ===
adapter driver hla
hla layout nulink
hla vid_pid 0x0416 0x511C
transport select swd

# === Target ===
swd newdap M487 cpu -expected-id 0x2BA01477
dap create M487.dap -chain-position M487.cpu
target create M487.cpu cortex_m -dap M487.dap

# === Work-area（燒錄用 RAM）===
M487.cpu configure -work-area-phys 0x20000000 -work-area-size 0x4000

# === Flash banks ===
# APROM (0x00000000)
flash bank M487.aprom numicro 0x00000000 0 0 0 M487.cpu
# LDROM (0x00100000)
flash bank M487.ldrom numicro 0x00100000 0 0 0 M487.cpu
# CONFIG (0x00300000)
flash bank M487.config numicro 0x00300000 0 0 0 M487.cpu

# === 燒錄流程 ===
reset halt
# 清除 Flash（可選）
flash erase_address 0x00000000 0x80000
# 燒錄 binary
flash write_image erase "path/to/HSUSBD_Mass_Storage_ShortPacket.bin" 0x00000000
# Reset
reset halt
```

### 4.5 ⚠️ 關鍵障礙：OpenOCD Flash 演算法

**⚠️ KEIL 的 FLM 演算法（M481_AP_512.FLM）是一個專有的二進制檔案**，包含完整的 Flash 程式化邏輯。OpenOCD 的 `numicro` driver 是通用的 Nuvoton Flash 驅動，可能：
- ✅ 可正常燒錄 APROM/LDROM/CONFIG
- ⚠️ 可能不完全支援 M487 的特殊 Flash 行為
- ⚠️ ShortPacket 功能可能涉及 HSUSBD USB 端的特殊處理，與 Flash 無直接關聯

### 4.6 ResetAndRun=0 的處理

KEIL `ResetAndRun=0` 表示燒錄後停在 Flash 起始位址，不自動執行。在 OpenOCD 中，燒錄後應使用 `reset halt` 停在 reset 向量，或燒錄後 `mdw 0x00000000` 驗證內容。

---

## 5. 總結與建議

### 5.1 主要發現

1. **晶片型號差異**：KEIL 專案以 `M487JIDAE` 為目標，但使用 `M481_AP_512.FLM` 演算法（512KB Flash）。M487 實際 Flash 大小需確認（可能是 256KB）。
2. **GCC Startup/LDS 完整**：BSP 已提供 `startup_M480.S` 和 linker scripts，GCC 編譯的基礎齊備。
3. **無 GCC Makefile**：需從零建立，無直接參考。
4. **OpenOCD Flash 支援**：使用內建 `numicro` driver，燒錄底層可行，但需驗證與 M487 Flash 的相容性。
5. **StdDriver 為原始碼**：非預編譯 .o，可直接加入 GCC Makefile 編譯。

### 5.2 建議行動項目

| 優先級 | 行動 | 說明 |
|--------|------|------|
| 🔴 高 | 確認 M487JIDAE Flash 大小 | 影響 linker script 選擇 |
| 🔴 高 | 建立 GCC Makefile | 以 StdDriver 建置流程為參考 |
| 🟡 中 | 驗證 OpenOCD numicro driver 燒錄 | 先在不燒錄模式下測試連線 |
| 🟡 中 | 確認 ShortPacket 功能實作位置 | USB 描述碼層，可能不涉 Flash |
| 🟢 低 | 研究 pyOCD 作為替代燒錄工具 | VSCode 已配置，燒錄較 OpenOCD 簡單 |

### 5.3 障礙標記

- ⚠️ **障礙 #1**：`build_gcc/Makefile` 不存在，無參考範本，需從零建立 Makefile
- ⚠️ **障礙 #2**：M487 Flash 大小未確認，影響 linker script 選擇
- ⚠️ **障礙 #3**：OpenOCD numicro driver 對 M487 的支援程度未經驗證

---

*報告產出：T001-ST1*
