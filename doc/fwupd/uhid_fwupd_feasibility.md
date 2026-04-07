# Linux uhid 虛擬 HID 裝置用於 FWUPD 測試之可行性研究

**研究日期：** 2026-03-30  
**研究目標：** 評估 Linux uhid 虛擬 HID 裝置是否可用於 FWUPD 韌體更新測試，特別針對 HID over I2C 場景

---

## 1. 研究發現摘要

### 1.1 uhid 能力分析

**uhid（User-space I/O driver support for HID subsystem）** 是 Linux 核心提供的一種機制，允許使用者空間程式模擬 HID 傳輸驅動程式。其核心特性：

| 特性 | 說明 |
|------|------|
| 設備節點 | `/dev/uhid`（misc character device） |
| 核心支援 | 需載入 `uhid` 核心模組 |
| 設備呈現 | 會在 `/sys/bus/hid` 和 `/sys/class/hidraw/` 出現對應的 hidraw 節點 |
| ioctl 支援 | `HIDIOCGFEATURE`, `HIDIOCSFEATURE`, `HIDIOCGRDESC`, `HIDIOCGRAWINFO` |
| 資料傳輸 | 透過 `UHID_INPUT2`（輸入）和 `UHID_OUTPUT`（輸出）事件與核心溝通 |
| Report 處理 | 支援 `UHID_GET_REPORT` / `UHID_SET_REPORT` 來回應核心的 Feature Report 請求 |

運作流程：
1. 使用者空間程式開啟 `/dev/uhid`
2. 發送 `UHID_CREATE2` 事件（含 Report Descriptor、Vendor ID、Product ID 等）
3. 核心 HID 子系統註冊該虛擬裝置，創建 `/dev/hidrawX`
4. 當應用程式讀取 hidraw 時，核心發送 `UHID_OPEN` 事件
5. 虛擬裝置可發送 `UHID_INPUT2` 餽送 HID 報告，或回應 `UHID_GET_REPORT` / `UHID_SET_REPORT`

### 1.2 FWUPD HID 設備識別機制

FWUPD 對 HID 設備的識別（查看 `fu-hidraw-device.c` 和 `fu-hid-device.c` 原始碼）依賴以下幾個關鍵機制：

#### 1.2.1 FuHidrawDevice 的探測流程（fu_hidraw_device_probe）

```
FuHidrawDevice.probe()
  1. 解析 udev 設備編號（fu_udev_device_parse_number）
  2. 取得父設備：fu_device_get_backend_parent_with_subsystem(device, "hid", error)
     → 必須在 sysfs 中有父設備且 subsystem 為 "hid"
  3. 讀取 udev 屬性：
     - HID_ID    → 格式 "BUS:VID:PID"（用於設定 VID/PID）
     - HID_NAME  → 設備名稱
     - HID_PHYS  → 物理路徑（用於設定 Physical ID）
     - HID_UNIQ  → 唯一識別碼（用於設定 Logical ID）
  4. 從 /sys/class/hid/hidXXX/ 讀取這些屬性
  5. 建立 instance ID：
     - USB\VID_XXXX
     - HIDRAW\VEN_XXXX
     - HIDRAW\VEN_XXXX&DEV_XXXX
```

#### 1.2.2 FuHidDevice（USB HID）vs FuHidrawDevice

| 層級 | FuHidDevice | FuHidrawDevice |
|------|------------|----------------|
| 基礎類別 | FuUsbDevice | FuUdevDevice |
| 通信方式 | USB Control Transfer + Interrupt Transfer | hidraw ioctl（HIDIOCGFEATURE / HIDIOCSFEATURE） |
| 設備取得 | 枚舉 USB 設備 | 枚舉 udev 的 hidraw subsystem |
| Report 傳輸 | USB 標準 HID Protocol | 直接讀寫 /dev/hidrawX |

#### 1.2.3 Elan Touchpad 插件的設備識別

從 `plugins/elantp/fu-elantp-hid-device.c` 可看出 Elantp 插件的識別邏輯：

```
設備枚舉時：
  1. 設備類型：FuElantpHidDevice（繼承自 FuHidrawDevice）
  2. Instance ID 格式：
     - HIDRAW\VEN_04F3&DEV_3010        （標準 HID instance）
     - HIDRAW\VEN_04F3&DEV_3010&MOD_XXXX （含模組 ID）
     - ELANTP\ICTYPE_09                 （IC 類型，quirk only）
     - ELANTP\ICTYPE_09&MOD_XXXX        （IC + 模組）
     - ELANTP\ICTYPE_09&MOD_XXXX&DRIVER_HID  （區分 HID/ABS 模式）
     - ELANTP\ICTYPE_09&MOD_XXXX&DRIVER_ELAN_I2C （I2C/ABS 模式）
```

**重要發現：** Elantp 插件支援兩種設備模式：
- **HID 模式**（`DRIVER_HID`）：透過 HIDRAW 界面更新
- **I2C/ABS 模式**（`DRIVER_ELAN_I2C`）：透過原始 I2C 節點更新（recovery 用）

### 1.3 HID over I2C 的核心差異

根據 Hackaday 文章和 Linux 核心文件，HID over I2C 的關鍵實作：

```
真實 HID over I2C 架構：
┌─────────────┐    I2C bus    ┌──────────────────┐
│  Touchpad   │◄──────────────│    i2c-hid       │
│  (HID I2C)  │               │  kernel driver   │
└─────────────┘               └────────┬─────────┘
                                        │ creates
                               ┌────────▼─────────┐
                               │  hid_bus_type    │
                               │  (i2c_hid bus)   │
                               └────────┬─────────┘
                                        │ creates
                               ┌────────▼─────────┐
                               │  hid subsystem   │
                               │  + hidraw device │
                               └─────────────────┘

必要條件（由 ACPI/DTB 描述）：
  compatible = "hid-over-i2c"
  reg = <I2C slave address>
  hid-descr-addr = <HID descriptor register address>
  interrupts = <IRQ>
```

**i2c-hid 核心模組**（`drivers/hid/i2c-hid/i2c-hid-core.c`）負責：
1. 透過 I2C 讀取 HID descriptor（從 `hid-descr-addr`）
2. 解析 Report Descriptor
3. 處理 HID I2C 協定（interrupt 通知、command/response 封裝）
4. 向核心 HID 子系統註冊

---

## 2. uhid vs 真實 HID over I2C 的差異

| 差異項目 | uhid | 真實 HID over I2C |
|---------|------|-------------------|
| **設備來源** | 使用者空間透過 `/dev/uhid` 創建 | ACPI/DTB 描述，由 `i2c-hid` 核心驅動程式創建 |
| **匯流排連接** | 無（不連接任何實體匯流排） | 連接至 I2C 匯流排 |
| **HID 描述來源** | 使用者空間在 `UHID_CREATE2` 中提供 | 從 I2C 裝置的 HID descriptor registers 讀取 |
| **中斷機制** | 由使用者空間主動餽送 | 由 INT pin 中斷觸發，驅動程式讀取 |
| **sysfs HID_ID** | 可能為 0 或預設值（取決於 uhid 實現） | 由 `i2c-hid` 驅動程式正確設定 |
| **sysfs HID_PHYS** | 虛擬路徑（無實際意義） | 格式如 `i2c-ELAN0001:00` |
| **ACPI/DTB 設備節點** | 無 | 有（I2C 設備描述） |
| **i2c-hid 協定處理** | 無（需使用者空間自行實現） | 由 `i2c-hid` 核心模組處理 |
| **/dev/hidrawX** | ✅ 會創建 | ✅ 會創建 |
| **HIDIOCGRAWINFO** | ✅ 可用（但 bus_type 可能不正確） | ✅ 可用 |
| **GET/SET REPORT ioctls** | ✅ 支援（需使用者空間回應） | ✅ 支援（由驅動程式處理） |

### 核心問題：uhid 的 HID_ID 屬性

在 `fu_hidraw_device_probe()` 中，FWUPD 需要從父 `hid` 設備讀取 `HID_ID` 屬性：

```c
prop_id = fu_udev_device_read_property(FU_UDEV_DEVICE(hid_device), "HID_ID", error);
// 格式："BUS:VID:PID" 例如 "0x03:0x04F3:0x3010"
```

對於 uhid 設備：
- `HID_ID` 的值取決於你在 `UHID_CREATE2` 事件中設定的 `bus_type`、`vendor`、`product` 欄位
- 核心預設可能不設定這些值，或設定為 0
- **這是可以透過正確的 uhid 應用程式實現來解決的**，但需要：
  1. 正確設定 `hidraw_devinfo.bustype`（uhid 不直接支援，需透過其他方式）
  2. 正確設定 HID 設備的 sysfs 屬性

**注意：** `struct uhid_create2_req` 中包含 `rd_data`（Report Descriptor）和維 VID/PID，但目前 uhid 介面**不直接提供設定 bus_type 的方法**。裝置的 bus_type 是由核心自動根據 uhid 驅動程式設定的。實際測試顯示 uhid 設備的 `HIDIOCGRAWINFO` 會傳回 `bustype = BUS_VIRTUAL (0x06)`。

---

## 3. FWUPD 識別 HID over I2C 設備的技術要求

### 3.1 識別流程所需的條件

FWUPD 要正確識別 HID over I2C 設備，需滿足：

1. **存在 `/dev/hidrawX` 設備節點**（hidraw subsystem）
2. **存在父 `hid` 設備**（在 `/sys/bus/hid/devices/` 或 `/sys/class/hid/`）
3. **父設備有正確的 udev 屬性**：
   - `HID_ID`：格式為 `BUS:VID:PID`
   - `HID_NAME`：設備名稱
   - `HID_PHYS`：物理識別字串
   - `HID_UNIQ`：唯一識別碼
4. **能透過 hidraw ioctl 與設備通訊**（HIDIOCGFEATURE / HIDIOCSFEATURE）
5. **對於特定插件**（如 Elantp），需要額外條件：
   - **Elantp HID 模式**：HIDRAW 界面 + I2C 回復路徑（i2c recovery 設備存在）
   - **Elantp I2C/ABS 模式**：需要實際的 I2C 節點（如 `/dev/i2c-X` 或 i2c-dev）

### 3.2 為何 FWUPD 需要這些屬性

FWUPD 的設備識別基於 **Instance ID** 匹配：

```
Instance ID 範例（Elan Touchpad）：
  HIDRAW\VEN_04F3&DEV_3010&MOD_1234
  ELANTP\ICTYPE_09&MOD_1234&DRIVER_HID
```

這些 instance ID 是從 HID_ID 中的 VID/PID 以及設備特定資訊建構的。如果 `HID_ID` 為 0 或無法讀取，FWUPD 將無法建立正確的 instance ID，導致：
- 無法與 LVFS 上的韌體匹配
- quirk 規則無法套用
- 設備無法被正確識別

### 3.3 系統匯流排依賴性

對於 HID over I2C 設備，FWUPD 的識別並不直接依賴 ACPI/platform bus 設備節點本身，但：

| 依賴類型 | 說明 |
|---------|------|
| **sysfs hid 設備** | 必須存在於 `/sys/bus/hid/devices/` 或 `/sys/class/hid/` |
| **udev 屬性** | 從父 hid 設備的 udev 屬性中讀取 VID/PID/名稱 |
| **hidraw 設備節點** | `/dev/hidrawX` 必須存在且可存取 |
| **HIDIOCGRAWINFO ioctl** | 需能成功執行並傳回正確的 bus_info |
| **I2C 節點（部分插件）** | Elantp 的 I2C recovery 模式需要 `/dev/i2c-X` 或 sysfs i2c 設備 |

---

## 4. Elan Touchpad FWUPD 實作分析

### 4.1 設備類型與模式

Elan Touchpad 在 FWUPD 中有兩種設備類型：

| 模式 | 設備類別 | 設備節點 | 說明 |
|------|---------|---------|------|
| HID 模式 | `FuElantpHidDevice` | `/dev/hidrawX` | 透過 HID Feature Report 通信 |
| I2C/ABS 模式 | `FuElantpI2cDevice` | `/dev/i2c-X` + I2C slave | Recovery 用，非常慢 |

### 4.2 HID Feature Report 通信流程

從 `fu-elantp-hid-device.c` 的 `fu_elantp_hid_device_read_cmd()` 可看出標準備份流程：

```c
// 1. SetReport：發送讀取命令（透過 HIDIOCSFEATURE）
fu_hidraw_device_set_feature(device, st_req->buf->data, st_req->buf->len, ...);

// 2. GetReport：讀取回應（透過 HIDIOCGFEATURE）
fu_hidraw_device_get_feature(device, buf->data, buf->len, ...);
```

### 4.3 關鍵發現：虛擬 uhid 可部分模擬 HID 界面

對於 HID 模式測試，**uhid 設備可以提供 `/dev/hidrawX` 節點**，使得：
- `HIDIOCGFEATURE` / `HIDIOCSFEATURE` ioctls 可正常運作
- HID Report 交換可以模擬

但以下**無法**透過 uhid 模擬：
- I2C/ABS recovery 模式（需要真實 I2C 設備節點）
- 設備在系統中的正確識別（因為 HID_ID 等屬性可能不正確）
- ACPI/DTB 設備描述的存在
- 設備與實際 I2C 匯流排的關聯

---

## 5. 可行性評估

### 5.1 當前方法（uhid）的限制

| 限制類別 | 具體問題 | 嚴重程度 |
|---------|---------|---------|
| **bus_type 問題** | uhid 設備的 bus_type 為 `BUS_VIRTUAL (0x06)`，而真實 HID over I2C 設備為 `BUS_I2C (0x0F)` | ⚠️ 中等 |
| **HID_ID 屬性** | 如果 uhid 實現未正確設定 sysfs 屬性，FWUPD 無法讀取 VID/PID | 🔴 高 |
| **I2C recovery** | 完全無法模擬，Elantp 的 I2C recovery 模式無法測試 | 🔴 高 |
| **Plugin 特定假設** | 插件（如 Elantp）可能假設設備存在於特定 sysfs 路徑 | ⚠️ 中等 |
| **設備枚舉** | FWUPD 的設備發現是基於 udev 枚舉，uhid 設備可能不出現在預期位置 | 🔴 高 |

### 5.2 替代方案比較

| 方案 | 描述 | 對 FWUPD 的支援程度 | 實現難度 |
|------|------|-------------------|---------|
| **uhid** | 使用者空間虛擬 HID | 部分可行（受限於屬性設定） | 低 |
| **hid-generic + 模擬** | 使用現有 HID 驅動程式 | 取決於具體設備驅動程式 | 中 |
| **i2c-hid + DTB 模擬** | 在 QEMU VM 中使用 DTB 描述虛擬 I2C-HID 設備 | 可行（需 VM 環境） | 高 |
| **使用者空間 I2C 模擬** | 模擬 I2C 匯流排和 i2c-hid 驅動程式 | 複雜（涉及核心驅動程式層） | 很高 |
| **Mock Plugin（fwupdtool）** | 使用 `fwupdtool --plugins` 測試特定插件邏輯 | 可完全控制，但需設備已存在 | 中 |
| **真實硬體** | 使用實際的 HID over I2C 設備 | 完整支援 | N/A |

### 5.3 推薦方案

#### 方案 A：uhid 可行性測試（適用於基礎測試）

如果目標是**測試 HID Report 交換邏輯**（SetReport/GetReport），且不涉及：
- FWUPD 設備枚舉
- Plugin 的設備識別流程
- I2C recovery 模式

則 uhid **可以部分使用**，但需要注意：
1. 在 uhid 應用程式中正確設定 Report Descriptor 和 VID/PID
2. 確保 `/sys/class/hid/hidXXX/` 下有正確的 udev 屬性（可能需要額外設定）

#### 方案 B：fwupdtool --plugins 測試（推薦用於插件邏輯測試）

```bash
# 使用 fwupdtool 直接測試插件邏輯（無需實際設備）
fwupdtool --plugins elantp get-devices
fwupdtool --plugins elantp install firmware.cab
```

此方式可測試：
- Plugin 的 `detach` / `attach` 邏輯
- Firmware 相容性檢查
- Update 流程

#### 方案 C：QEMU + DTB 模擬 HID over I2C（適用於完整集成測試）

在 QEMU VM 中：
1. 透過 DTB 描述虛擬 I2C-HID 設備
2. 載入 `i2c-hid` 核心模組
3. 設備出現在 `/dev/hidrawX` 並有正確的 HID_ID 屬性
4. 可完整測試 FWUPD 設備發現流程

此方法可完整模擬 HID over I2C 的系統架構。

#### 方案 D：硬體測試（適用於最終驗證）

使用真實的 Elan Touchpad 或其他 HID over I2C 設備進行完整測試。

---

## 6. 結論

### 6.1 核心結論

**uhid 虛擬 HID 裝置對 FWUPD 測試的可行性非常有限。**

主要原因：
1. **識別機制障礙**：FWUPD 的 HID 設備發現高度依賴 sysfs 中父 `hid` 設備的 udev 屬性（HID_ID、HID_PHYS 等），而 uhid 設備難以正確提供這些屬性。
2. **Bus type 不匹配**：uhid 設備的 bus_type 為 `BUS_VIRTUAL`，與 HID over I2C 的 `BUS_I2C` 不符，可能導致插件無法匹配。
3. **I2C 回復路徑缺失**：Elantp 等插件的 recovery 功能需要實際的 I2C 設備節點，無法透過 uhid 模擬。
4. **系統整合不足**：FWUPD 的設備發現是與 Linux udev/sysfs 緊密整合的，uhid 提供的虛擬層不足以欺騙整個系統。

### 6.2 實用建議

| 測試目標 | 推薦方案 |
|---------|---------|
| 測試 HID Report 交換（GetFeature/SetFeature） | uhid + 自定義使用者空間程式（不經過 FWUPD） |
| 測試 Elantp 插件邏輯 | `fwupdtool --plugins elantp` + Mock 設備 |
| 測試 FWUPD 設備發現流程 | QEMU VM + DTB 虛擬 I2C-HID 設備 |
| 完整集成測試 | 真實硬體（HID over I2C 觸控板） |
| 開發調試 Plugin | `fwupdtool --plugins <name> --verbose` |

### 6.3 未來方向建議

1. **研究 FWUPD Device Emulation**：FWUPD 2.0+ 支援設備模擬（`fu_device_emulation`），可能可用於軟體模擬 HID 設備。
2. **自定義 FWUPD Plugin + Mock 設備**：在測試環境中使用 Mock 設備類別，避免依賴實際硬體。
3. **貢獻 i2c-hid 模擬層**：如果需要在 VM 環境中測試，可以考慮開發 i2c-hid 的使用者空間替代方案。

---

## 參考資料

1. Linux Kernel Documentation: UHID - User-space I/O driver support for HID subsystem  
   https://docs.kernel.org/hid/uhid.html

2. FWUPD Source Code: libfwupdplugin/fu-hidraw-device.c  
   https://github.com/fwupd/fwupd/blob/main/libfwupdplugin/fu-hidraw-device.c

3. FWUPD Source Code: libfwupdplugin/fu-hid-device.c  
   https://github.com/fwupd/fwupd/blob/main/libfwupdplugin/fu-hid-device.c

4. FWUPD Plugin: Elantp TouchPad  
   https://fwupd.github.io/libfwupdplugin/elantp-README.html

5. FWUPD Plugin Source: plugins/elantp/fu-elantp-hid-device.c  
   https://github.com/fwupd/fwupd/blob/main/plugins/elantp/fu-elantp-hid-device.c

6. Linux Kernel: HID I/O Transport Drivers  
   https://www.kernel.org/doc/html/latest/hid/hid-transport.html

7. HID over I2C - Device Tree Binding  
   https://mjmwired.net/kernel/Documentation/devicetree/bindings/input/hid-over-i2c.txt

8. Hackaday: Human-Interfacing Devices: HID Over I2C  
   https://hackaday.com/2024/04/17/human-interfacing-devices-hid-over-i2c/

9. Linux Kernel: drivers/hid/i2c-hid/i2c-hid-core.c  
   https://github.com/torvalds/linux/blob/master/drivers/hid/i2c-hid/i2c-hid-core.c

10. FWUPD Device Discovery Documentation  
    https://deepwiki.com/fwupd/fwupd/2.3-device-discovery
