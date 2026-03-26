# M487 USB Storage + HID I2C Bridge

Win32 C++ 工具，包含：
1. **M487_ScsiTool**: USB Mass Storage CLI 工具 + Vendor Commands + I2C 控制
2. **HID I2C Bridge**: 獨立的 HID I2C 控制工具（適用於 Composite Firmware）

---

## 專案結構

```
M487_ScsiTool/
├── README.md                # 本文件
├── CMakeLists.txt          # M487_ScsiTool 建置腳本
├── include/                # M487_ScsiTool 標頭
├── src/                    # M487_ScsiTool 原始碼
├── firmware/              # M487 韌體原始碼
│   ├── i2c_control.c/h   # I2C 控制（可整合進 MSC firmware）
│   ├── composite/         # Composite Device (MSC + HID I2C)
│   │   ├── main.c
│   │   ├── hid_i2c.c/h
│   │   ├── usb_descriptors.c/h
│   │   └── README.md
│   └── README.md          # 韌體整合說明
└── hid_bridge/           # HID I2C Bridge 主機工具
    ├── CMakeLists.txt
    ├── include/
    └── src/
```

---

## 工具 1: M487_ScsiTool (USB Mass Storage + I2C)

透過 USB Mass Storage Vendor Command 發送 I2C 控制命令。

### 功能
- **SCSI READ/WRITE**: 讀寫 Flash storage
- **Vendor Commands**: I2C 控制（透過 SCSI vendor opcode 0xC0）
- **速度測試**: 測試傳輸速度

### 建置
```powershell
cd M487_ScsiTool
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 指令
```
enumerate              # 列舉裝置
connect               # 連接
i2c-write <addr> <hex>  # I2C 寫入
i2c-read <addr> <len>   # I2C 讀取
i2c-writeread <addr> <whex> <rlen>  # I2C 寫入後讀取
speed-read [sectors]     # 讀取速度測試
speed-write [sectors]    # 寫入速度測試
```

---

## 工具 2: HID I2C Bridge (獨立 HID 工具)

適用於 Composite Firmware（MSC + HID I2C），使用標準 HID Reports。

### 功能
- **I2C Scan**: 掃描 I2C bus 上的裝置
- **I2C Write/Read**: 標準 I2C 通訊
- **I2C Write+Read**: 先寫入暫存器位址再讀取

### 建置
```powershell
cd hid_bridge
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### 指令
```
enumerate, enum   # 列舉 HID I2C Bridge 裝置
connect           # 連接到第一個裝置
scan             # I2C bus 掃描
write <addr> <hex>  # I2C 寫入
read <addr> <len>    # I2C 讀取
writeread <addr> <whex> <rlen>  # 寫入後讀取
```

---

## Firmware 選項

### 選項 A: MSC + Vendor Commands（現有）
- 在 USB Mass Storage 基礎上新增 vendor command (0xC0)
- 較簡單，但 HID 功能需透過 vendor command
- 使用 M487_ScsiTool

### 選項 B: Composite Device（新建議）
- USB Mass Storage (MSC) + HID I2C Bridge 同時存在
- Windows 自動識別 HID，plug-and-play
- 使用 HID I2C Bridge 工具
- 需要完整實作 USB descriptors 和 HID class

---

## USB 參數

| 參數 | MSC | Composite |
|------|-----|-----------|
| VID | 0x0416 | 0x0416 |
| PID | 0x501E | 0x5020 |
| Interface 0 | Mass Storage | MSC |
| Interface 1 | - | HID I2C Bridge |

## I2C 參數

| 參數 | 值 |
|------|-----|
| I2C Controller | UI2C0 (USCI_I2C) |
| Pins | PE2=CLK, PE3=DAT0 |
| Speed | 100 kHz (預設) |
| Max Write | 60 bytes |
| Max Read | 62 bytes |

---

## 韌體建置

請參考 `firmware/` 資料夾中的 README.md


Win32 C++ CLI 工具，透過 WinUSB 發送 SCSI 命令到 M487 USB Storage 設備。

## 功能

- **SCSI READ(10)**: 讀取指定 LBA 的 sectors
- **SCSI WRITE(10)**: 寫入資料到指定 LBA
- **SCSI INQUIRY**: 查詢設備資訊
- **SCSI READ CAPACITY**: 查詢總容量
- **Vendor Command (0xC0)**: 讀取設備識別字串 "ELAN-USB-I2C-BRIDGE-V0.1"

## 需求

- Windows 10/11
- Visual Studio 2019+ 或 MinGW-w64
- CMake 3.15+
- M487 設備以 USB Mass Storage 模式運行（VID=0x0416, PID=0x501E）

## 建置

### 使用 CMake + Visual Studio

```powershell
cd D:\AiWorkSpace\M487_ScsiTool
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

執行檔會產生在 `build\bin\Release\M487USBStorage.exe`

### 使用 MinGW

```powershell
cd D:\AiWorkSpace\M487_ScsiTool
mkdir build && cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```

執行檔會產生在 `build\bin\M487USBStorage.exe`

## 使用方式

```
M487> enumerate          # 列出可用的 M487 USB Storage 設備
M487> connect            # 連接到設備
M487> info               # 顯示設備資訊（容量、VID/PID 等）
M487> inquiry            # SCSI INQUIRY
M487> read 0             # 讀取 LBA 0（1 sector）
M487> read 0 4           # 讀取 LBA 0-3（4 sectors）
M487> write 0 0x55AA     # 寫入 0x55AA pattern 到 LBA 0
M487> vendor             # Vendor command：讀取設備字串
M487> dump 0 2           # Hex dump LBA 0-1
M487> fill 0 10 0xAA55   # 填滿 LBA 0-9，每個 byte 為 0xAA55 pattern
M487> help               # 顯示所有指令
M487> exit               # 離開
```

## 指令說明

| 指令 | 說明 |
|------|------|
| `enumerate` | 列舉 VID=0x0416, PID=0x501E 的設備 |
| `connect` | 連接到設備 |
| `disconnect` | 中斷連線 |
| `info` | 顯示設備資訊（總容量、sector size） |
| `inquiry` | SCSI INQUIRY（查詢廠商、產品名稱） |
| `read <lba> [count]` | 讀取指定 LBA，預設 count=1 |
| `write <lba> <hex>` | 寫入 hex pattern 到指定 LBA（1 sector） |
| `vendor` | Vendor command (0xC0)，讀取 "ELAN-USB-I2C-BRIDGE-V0.1" |
| `dump <lba> [count]` | Hex dump 指定 sectors |
| `fill <lba> <count> <hex>` | 以 hex pattern 填滿多個 sectors |
| `help` | 顯示說明 |
| `exit` | 結束程式 |

## SCSI CDB 結構

### READ(10) - Opcode 0x28

| Byte | 內容 |
|------|------|
| 0 | Opcode = 0x28 |
| 1 | Obsolete |
| 2-5 | LBA (MSB first) |
| 6 | Reserved |
| 7-8 | Transfer Length in sectors (MSB first) |
| 9 | Control |

### WRITE(10) - Opcode 0x2A

| Byte | 內容 |
|------|------|
| 0 | Opcode = 0x2A |
| 1 | Obsolete |
| 2-5 | LBA (MSB first) |
| 6 | Reserved |
| 7-8 | Transfer Length in sectors (MSB first) |
| 9 | Control |

### Vendor Read (0xC0) - 自訂

| Byte | 內容 |
|------|------|
| 0 | Opcode = 0xC0 |
| 1 | Sub-cmd: 0x00 = Get Device String |
| 2-3 | Transfer Length (MSB first) |
| 4-8 | Reserved |
| 9 | Control |

**預期回傳**: `"ELAN-USB-I2C-BRIDGE-V0.1"` (24 bytes 含 null terminator)

## M487 USB Storage 預設參數

根據 `USBD_Mass_Storage_Flash` 範例：

| 參數 | 值 |
|------|-----|
| VID | 0x0416 (Nuvoton) |
| PID | 0x501E |
| 總容量 | 64 KB (128 sectors × 512 bytes) |
| Sector Size | 512 bytes |
| Max Transfer | 65535 sectors (理論值) |
| 實際限制 | 128 sectors (受限於 DataFlash 大小) |

## 資料夾結構

```
M487_ScsiTool/
├── CMakeLists.txt        # CMake 建置腳本
├── README.md             # 本文件
├── include/
│   ├── usb.h           # WinUSB 介面（動態載入 WinUSB DLL）
│   ├── scsi.h          # SCSI CDB 結構
│   └── device.h        # 裝置操作封裝
└── src/
    ├── usb.cpp         # WinUSB 列舉 + Bulk transfer
    ├── scsi.cpp        # SCSI CDB 建構 + byte-swap
    ├── device.cpp      # CBW/CSW + SCSI 命令流程
    └── main.cpp        # CLI 主程式
```

## 注意事項

1. **USB 驅動程式**: 設備需使用 WinUSB 驅動。可使用 Zadig 或 inf-wizard 替換驅動。
2. **Vendor Command**: M487 韌體需要實作 opcode 0xC0 的 handler，回傳 "ELAN-USB-I2C-BRIDGE-V0.1" 字串。
3. **錯誤處理**: CSW Status = 0x00 表示成功，0x01 表示命令失敗，0x02 表示 Phase Error。
4. **權限**: 部分 USB 操作可能需要系統管理員權限。

## 授權

使用前請參閱 Nuvoton M480 BSP 授權條款。
