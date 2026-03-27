# HID Tool (hidtool) - CLI 設計規格書

## 1. 概述

`hidtool` 是一個運行於 Windows 的命令列工具，透過 USB HID 介面與 M487 HID-over-I2C 橋接器通訊。

### 1.1 設計目標

- 提供簡易的 I2C 讀寫介面（透過 USB HID 封包）
- 支援 HID 裝置列舉與選取
- 支援熱插拔監聽
- 提供詳細的錯誤訊息
- 可選的除錯日誌功能

### 1.2 系統需求

- Windows 10/11
- USB HID 裝置（M487 HID-over-I2C Bridge）
- libusb 或 WinUSB 驅動程式

---

## 2. CLI 指令格式

### 2.1 指令總覽

```
hidtool [全域選項] <指令> [指令選項] [參數]
```

### 2.2 全域選項

| 選項 | 說明 |
|------|------|
| `-v, --verbose` | 輸出詳細除錯資訊 |
| `-q, --quiet` | 安靜模式（只輸出結果） |
| `-h, --help` | 顯示說明 |
| `-V, --version` | 顯示版本 |

### 2.3 指令清單

#### 2.3.1 `hidtool device list`

列出所有已連接的 HID 裝置。

**輸出格式：**
```
[INFO] Found N HID device(s)

  #0: VID=0x04F3 PID=0x0732 "M487 HID-I2C Bridge"
       Serial: ABCD1234
       Path: \\?\hid#vid_04f3&pid_0732#...
  #1: VID=0x0000 PID=0x0000 "Unknown Device"
       ...
```

**退出碼：**
- `0` - 成功
- `1` - 無找到任何 HID 裝置

---

#### 2.3.2 `hidtool device select <index>`

選取要操作的 HID 裝置。

**參數：**
- `index` - 裝置索引（由 `device list` 取得）

**輸出格式：**
```
[INFO] Selected device #0: VID=0x04F3 PID=0x0732
```

**退出碼：**
- `0` - 成功
- `1` - 裝置索引無效

**範例：**
```batch
hidtool device list
hidtool device select 0
```

---

#### 2.3.3 `hidtool read <i2c_addr> <addr> [len]`

從 I2C 從屬裝置讀取資料。

**參數：**
- `i2c_addr` - I2C 從屬位址（7-bit，例：`0x50`）
- `addr` - 讀取起始位址（8-bit）
- `len` - 讀取位元組數（可選，預設=1，最大=255）

**輸出格式：**
```
[INFO] I2C read: addr=0x50, reg=0x10, len=4
[DATA] 01 02 03 04
```

**錯誤格式：**
```
[ERROR] I2C read failed: NAK received (device not responding)
[ERROR] I2C read failed: bus error
```

**退出碼：**
- `0` - 成功
- `1` - 參數錯誤
- `2` - I2C NAK（從屬無回應）
- `3` - I2C bus error
- `4` - USB 通訊錯誤

**範例：**
```batch
hidtool read 0x50 0x10        # Read 1 byte from reg 0x10
hidtool read 0x50 0x10 16     # Read 16 bytes from reg 0x10
```

---

#### 2.3.4 `hidtool write <i2c_addr> <addr> <data>...`

寫入資料至 I2C 從屬裝置。

**參數：**
- `i2c_addr` - I2C 從屬位址（7-bit）
- `addr` - 寫入起始位址（8-bit）
- `data` - 要寫入的位元組（1-255 個，可為 `0xNN` 或 `NN`）

**輸出格式：**
```
[INFO] I2C write: addr=0x50, reg=0x10, len=4
[OK]   4 bytes written
```

**錯誤格式：**
```
[ERROR] I2C write failed: NAK received
[ERROR] I2C write failed: address not acknowledged
```

**退出碼：**
- `0` - 成功
- `1` - 參數錯誤
- `2` - I2C NAK
- `3` - I2C bus error
- `4` - USB 通訊錯誤

**範例：**
```batch
hidtool write 0x50 0x10 0xAB          # Write 1 byte
hidtool write 0x50 0x10 0xAB 0xCD 0xEF  # Write 3 bytes
```

---

#### 2.3.5 `hidtool reg read <reg>`

讀取橋接器本身的 HID 描述符暫存器（用於診斷）。

**參數：**
- `reg` - 暫存器位址（8-bit）

**輸出格式：**
```
[INFO] Register read: reg=0x00
[DATA] 01
```

---

#### 2.3.6 `hidtool reg write <reg> <data>`

寫入橋接器本身的控制暫存器。

**參數：**
- `reg` - 暫存器位址（8-bit）
- `data` - 要寫入的資料（8-bit）

---

#### 2.3.7 `hidtool log [on|off|export]`

控制除錯日誌。

**參數：**
- `on` - 啟用日誌輸出
- `off` - 停用日誌輸出
- `export` - 匯出日誌至檔案

**輸出格式：**
```
[INFO] Log enabled
[INFO] Log disabled
[INFO] Log exported to hidtool_log.txt
```

---

#### 2.3.8 `hidtool monitor [interval_ms]`

監聽並顯示 USB HID 報告（熱插拔偵測）。

**參數：**
- `interval_ms` - 監聽間隔（可選，預設=100ms）

**輸出格式：**
```
[INFO] Monitoring HID reports (Ctrl+C to stop)...
[TIME] +0.100s: EP1 IN: 01 02 03 04 05 06 07 08
[TIME] +0.210s: EP1 IN: 05 06 07 08 09 0A 0B 0C
[INFO] Device disconnected
[INFO] Device connected: VID=0x04F3 PID=0x0732
```

---

## 3. HID Report 格式

### 3.1 Host → Device（OUT）

| Offset | Size | 說明 |
|--------|------|------|
| 0 | 1 | Report ID = 0x01 |
| 1 | 1 | I2C Address (7-bit) |
| 2 | 1 | Operation: 0x01=Write, 0x02=Read |
| 3 | 1 | Register/Offset Address |
| 4 | 1 | Length (for read) |
| 5..63 | N | Data (for write) |

### 3.2 Device → Host（IN）

| Offset | Size | 說明 |
|--------|------|------|
| 0 | 1 | Report ID = 0x01 |
| 1 | 1 | Status: 0x00=OK, 0x01=NAK, 0x02=Error |
| 2 | 1 | Length |
| 3..63 | N | Data |

---

## 4. 錯誤訊息格式

所有錯誤訊息遵循以下格式：

```
[ERROR] <error_type>: <detailed_description>
```

### 4.1 錯誤類型

| 錯誤類型 | 說明 |
|----------|------|
| `Invalid argument` | 命令列參數無效 |
| `Device not found` | 指定索引的 HID 裝置不存在 |
| `USB error` | USB 通訊底層錯誤（描述具體 WinUSB/libusb 錯誤碼） |
| `I2C NAK` | I2C 從屬裝置未回應 |
| `I2C bus error` | I2C bus 錯誤（如：arbitration loss） |
| `I2C timeout` | I2C 操作超時 |
| `Protocol error` | HID report 格式錯誤 |
| `Device disconnected` | 裝置在操作期間斷開連接 |

### 4.2 詳細錯誤範例

```
[ERROR] USB error: WinUSB error 0x00000001: Device not found
[ERROR] I2C NAK: No acknowledgment from slave @ 0x50
[ERROR] I2C timeout: Bus stuck for >1000ms during write
[ERROR] Protocol error: Expected 8 bytes, got 4 bytes in report
[ERROR] Device disconnected: USB connection lost during transfer
```

---

## 5. Hotplug 監聽機制

### 5.1 行為描述

- 程式啟動時枚舉所有 VID/PID 符合的 HID 裝置
- 監聽執行期間，偵測 USB 裝置的插拔事件
- 偵測到事件時，輸出 `[INFO] Device connected/disconnected` 並可執行 callback

### 5.2 多裝置支援

當有多個符合 VID/PID 的裝置連接時，預設選擇第一個。可透過 `device select` 指令切換。

### 5.3 斷線處理

- 寫入時斷線：回傳 `[ERROR] Device disconnected`
- 讀取時斷線：回傳 `[ERROR] Device disconnected`
- `monitor` 模式會持續報告連線狀態

---

## 6. VID/PID

| 欄位 | 值 | 說明 |
|------|-----|------|
| VID | `0x1234` | （暫定，待更新） |
| PID | `0x5678` | （暫定，待更新） |
| Usage Page | `0xFF00` | Vendor-defined |
| Usage | `0x01` | HID-over-I2C Bridge |

> ⚠️ VID/PID 待正式定義後更新。

---

## 7. 附錄：退出碼對照表

| 退出碼 | 符號常數 | 說明 |
|--------|----------|------|
| 0 | `HIDTOOL_OK` | 操作成功 |
| 1 | `HIDTOOL_ERR_ARG` | 命令列參數錯誤 |
| 2 | `HIDTOOL_ERR_NOT_FOUND` | 找不到 HID 裝置 |
| 3 | `HIDTOOL_ERR_I2C_NAK` | I2C NAK |
| 4 | `HIDTOOL_ERR_I2C_TIMEOUT` | I2C 超時 |
| 5 | `HIDTOOL_ERR_I2C_BUS` | I2C bus 錯誤 |
| 6 | `HIDTOOL_ERR_USB` | USB 通訊錯誤 |
| 7 | `HIDTOOL_ERR_PROTOCOL` | 通訊協定錯誤 |
| 8 | `HIDTOOL_ERR_DISCONNECTED` | 裝置已斷線 |
| 99 | `HIDTOOL_ERR_UNKNOWN` | 未知錯誤 |
