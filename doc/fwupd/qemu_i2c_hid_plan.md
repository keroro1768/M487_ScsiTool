# QEMU + 虛擬 I2C-HID DTB 方案規劃

**研究日期：** 2026-03-30  
**研究目標：** 在 QEMU 虛擬機中建立 I2C-HID 設備，使 FWUPD 能正確識別並測試  
**前導研究：** `uhid_fwupd_feasibility.md`（uhid 方案已被判定可行性不足）

---

## 1. 研究發現摘要

### 1.1 QEMU 原生 I2C-HID 模擬現況

**關鍵發現：QEMU 目前沒有原生的 I2C-HID 設備模擬器。**

QEMU 的 `hw/i2c/` 目錄中存在的 I2C 設備：

| 檔案 | 說明 |
|------|------|
| `core.c` | I2C 匯流排核心（master/slave 抽象） |
| `smbus_eeprom.c` | SMBus EEPROM（AT24C64 等） |
| `smbus_slave.c` | 通用 SMBus slave 設備框架 |
| `imx_i2c.c` | i.MX I2C 控制器 |
| `aspeed_i2c.c` | Aspeed I2C 控制器 |
| `bcm2835_i2c.c` | BCM2835 I2C 控制器 |
| `omap_i2c.c` | OMAP I2C 控制器 |
| `arm_sbcon_i2c.c` | ARM PrimeCell SBSCN I2C |

**QEMU `hw/input/` 中也沒有 I2C-HID 設備。** 僅有：
- `hid.c`（USB HID 設備）
- `adb.c`（Apple Desktop Bus）
- `ps2.c`（PS/2 鍵盤滑鼠）
- `pl050.c`（ARM PrimeCell PS/2）

**結論：** 若要透過 QEMU 模擬 I2C-HID 設備，**必須自行開發 QEMU 設備模型**（custom device model），或採用其他替代方案。

---

## 2. 方案比較

### 2.1 架構選擇矩陣

| 架構 | 設備描述機制 | DTB 支援 | 實作難度 | I2C-HID 模擬可行性 |
|------|------------|---------|---------|------------------|
| **ARM (virt)** | DTB（自動生成 + 自定義覆寫） | ✅ 完整 | 高 | 可行（需 custom device + DTB） |
| **x86 (Q35)** | ACPI + PCI | ⚠️ 需自定義 ACPI | 極高 | 可行但複雜 |
| **RISC-V (virt)** | DTB | ✅ 完整 | 高 | 可行但少見 |
| **ARM (vexpress)** | DTB | ✅ 完整 | 高 | 可行（已停更） |

### 2.2 推薦架構：ARM virt

**選擇理由：**
1. `virt` board 是 QEMU 官方推薦的通用虛擬 ARM 平台
2. 支援完整 DTB 描述，可在 QEMU 命令列指定自定義 DTB
3. 支援 KVM 加速（如在 ARM 主機上執行）
4. I2C 控制器可透過 `-device` 掛載
5. 較 x86 ACPI 更易於注入自定義設備描述

---

## 3. 虛擬 I2C-HID 設備的系統架構

### 3.1 目標架構圖

```
Host 層（QEMU 模擬器）
┌─────────────────────────────────────────────┐
│  QEMU 模擬器                                 │
│  ┌──────────────┐    ┌────────────────────┐  │
│  │  imx-i2c     │    │  custom-i2c-hid    │  │
│  │  (I2C master)│────│  device model      │  │
│  └──────────────┘    │  (QEMU device)    │  │
│                       └────────────────────┘  │
│                             │                 │
│                      I2C bus                  │
└─────────────────────────────│─────────────────┘
                              │
VM 內核層（Linux guest）
┌─────────────────────────────│─────────────────┐
│  /dev/hidraw0  ←── hidraw subsystem           │
│         ↑                                      │
│  ┌──────┴────────┐                           │
│  │  i2c-hid       │  ←── hid-over-i2c driver  │
│  │  (drivers/hid/ │                           │
│  │   i2c-hid/)    │                           │
│  └──────┬─────────┘                           │
│         │                                      │
│  ┌──────┴─────────┐                           │
│  │  I2C bus        │                           │
│  │  (i2c-dev)      │  ←── standard i2c bus    │
│  └─────────────────┘                          │
└────────────────────────────────────────────────┘
```

### 3.2 為何需要自定義 QEMU 設備模型

`i2c-hid` Linux 驅動期望：
1. **I2C 匯流排**：存在 `/dev/i2c-X` 或 sysfs i2c 設備
2. **HID Descriptor**：從 I2C slave 的特定 registers 讀取（HID over I2C 協定的 0x1D / 0x1F register 地址）
3. **Report Descriptor**：從設備讀取後交由 HID 子系統解析
4. **中斷機制**：設備透過 INT pin 通知主機

QEMU 的標準 SMBus EEPROM (`smbus_eeprom`) 無法模擬這些行為。

---

## 4. DTB 設計（I2C-HID 設備節點）

### 4.1 HID over I2C 的 Device Tree Binding

根據 Linux kernel 文件 `Documentation/devicetree/bindings/hid/hid-over-i2c.txt`：

```dts
/* 標準 HID over I2C DTB 節點 */
&i2c0 {
    status = "okay";

    /* Elan Touchpad 範例 */
    elan touchscreen@10 {
        compatible = "hid-over-i2c";
        reg = <0x10>;                    /* I2C 從設備地址 0x10 */
        hid-descr-addr = <0x1F>;         /* HID descriptor 位於 register 0x1F */
        interrupts-extended = <&gpio 23 IRQ_TYPE_LEVEL_LOW>;  /* 中斷 */

        /* 可選：供應商/產品識別（部分驅動依賴） */
        vid = <0x04F3>;                  /* Elan VID */
        pid = <0x3010>;                 /* Elan PID */
    };
};
```

### 4.2 DTB 節點關鍵欄位說明

| 欄位 | 必要性 | 說明 |
|------|-------|------|
| `compatible` | 必要 | 必須為 `"hid-over-i2c"` 或 `"hid-over-i2c触摸屏"` |
| `reg` | 必要 | I2C 從設備位址（7-bit 格式） |
| `hid-descr-addr` | 必要 | HID descriptor 的 I2C register 位址 |
| `interrupts-extended` | 必要 | GPIO 中斷描述（EXTENDED 格式） |
| `vid` | 可選 | Vendor ID（部分設備需要） |
| `pid` | 可選 | Product ID（部分設備需要） |

### 4.3 虛擬設備 DTB 節點設計（模擬用）

```dts
/* QEMU ARM virt 的 I2C bus 節點 */
&i2c0 {
    status = "okay";
    clock-frequency = <100000>;

    /* 虛擬 I2C-HID 觸控板（用於 FWUPD 測試） */
    virt-touchpad@15 {
        compatible = "elan,em欺詐tep5510";  /* 欺騙驅動程式 */
        reg = <0x15>;                       /* I2C 地址 0x15 */
        hid-descr-addr = <0x1F>;            /* HID descriptor 寄存器 */
        interrupts-extended = <&gpio 22 IRQ_TYPE_EDGE_FALLING>;
        
        /* 欺騙 Elantp 插件的額外屬性 */
        vid = <0x04F3>;
        pid = <0x3010>;
        elan,module-id = <0x1234>;
    };
};
```

### 4.4 合併自定義 DTB 的方法

QEMU ARM virt board 支援傳入自定義 DTB：

```bash
# 方法 1：直接傳入 DTB
qemu-system-aarch64 \
    -dtb custom-i2c-hid.dtb \
    ...

# 方法 2：修改 QEMU 自動生成的 DTB（使用 device tree compiler）
dtc -I dtb -O dts /path/to/auto-generated.dtb > extracted.dts
# 手動編輯後重新編譯
dtc -I dts -O dtb custom.dtb extracted.dts
```

---

## 5. QEMU 啟動命令設計

### 5.1 ARM virt 基本命令

```bash
qemu-system-aarch64 \
    -machine virt,accel=tcg,gic-version=3 \
    -cpu cortex-a53 \
    -m 2G \
    -kernel Image \
    -append "console=ttyAMA0 root=/dev/vda rw" \
    -dtb custom-i2c-hid.dtb \
    -drive file=rootfs.ext4,format=raw,id=hd0 \
    -device virtio-blk-pci,drive=hd0 \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -nographic
```

### 5.2 掛載 I2C 控制器的 QEMU 命令

ARM virt board 的 I2C 控制器需要確認設備樹中有對應節點。以下範例使用 Aspeed I2C 控制器：

```bash
qemu-system-aarch64 \
    -machine virt,accel=tcg \
    -cpu cortex-a53 \
    -m 2G \
    -kernel Image \
    -append "console=ttyAMA0 root=/dev/vda rw i2c-dev.dummy_bus=0" \
    -dtb custom-i2c-hid.dtb \
    -drive file=rootfs.ext4,format=raw,id=hd0 \
    -device virtio-blk-pci,drive=hd0 \
    -device aspeed-i2c,id=i2c-bus-0 \
    -nographic
```

### 5.3 完整 QEMU 命令（整合 I2C + DTB）

```bash
# 環境設定
KERNEL="Image"
INITRD="initramfs.img"
ROOTFS="rootfs.ext4"
DTB="custom-virt-i2c-hid.dtb"

# QEMU 啟動命令
qemu-system-aarch64 \
    -machine virt,virtualization=on,gic-version=3 \
    -cpu max \
    -m 4G \
    -kernel "$KERNEL" \
    -initrd "$INITRD" \
    -append "console=ttyAMA0 root=/dev/vda rw quiet i2c_hid.debug=2" \
    -dtb "$DTB" \
    -drive file="$ROOTFS",format=raw,if=none,id=hd0 \
    -device virtio-blk-pci,drive=hd0 \
    -netdev user,id=net0,hostfwd=tcp::2222-:22 \
    -device virtio-net-pci,netdev=net0 \
    -device virtio-gpu-pci \
    -serial mon:stdio \
    -nographic
```

---

## 6. Linux Kernel 設定

### 6.1 必要的 Kernel Config 選項

```makefile
# I2C 核心支援
CONFIG_I2C=y
CONFIG_I2C_CHARDEV=y

# I2C HID（核心！！！）
CONFIG_I2C_HID=y
CONFIG_I2C_HID_CORE=y

# 對應的 I2C 控制器驅動（根據 QEMU 模擬的控制器選擇）
CONFIG_I2C_IMX=y          # i.MX I2C (if using imx-i2c in QEMU)
CONFIG_I2C_ASPEED=y       # Aspeed I2C (if using aspeed-i2c in QEMU)
CONFIG_I2C_BCM2835=y      # BCM2835 I2C

# HID 核心支援
CONFIG_HID=y
CONFIG_HID_GENERIC=y
CONFIG_HIDRAW=y

# 特定 HID 設備驅動（如果需要測試特定設備）
CONFIG_TOUCHSCREEN_ELAN_I2C=y  # Elan Touchpad I2C driver

# 虛擬 HID 設備（輔助測試用）
CONFIG_UHID=y

# Sysfs 和 udev
CONFIG_SYSFS=y
CONFIG_DEBUG_DEVRES=y
```

### 6.2 驗證 Kernel 模組載入

```bash
# 確認 i2c-hid 模組存在
lsmod | grep i2c_hid

# 確認 i2c 控制器被識別
ls /sys/bus/i2c/devices/

# 確認 i2c-hid 設備已創建
ls /sys/bus/i2c-hid/
find /sys/bus/i2c-hid/ -name "hid-*"

# 查看設備的 HID_ID
cat /sys/bus/i2c-hid/devices/hid-*/hid_id
# 輸出格式： bus:vendor:product (例如：0x06:0x04F3:0x3010)
```

---

## 7. FWUPD 識別流程與驗證

### 7.1 FWUPD 枚舉 I2C-HID 設備的關鍵路徑

```
/dev/hidrawX
  └── sys/class/hidraw/hidrawX
        └── device → ../../device (父設備)
              └── ../hid/hidXXX (父 hid 設備)
                    ├── HID_ID (sysfs 屬性：bus:vid:pid)
                    ├── HID_NAME (sysfs 屬性)
                    ├── HID_PHYS (sysfs 屬性)
                    └── uevent (包含 HID 事件)
```

### 7.2 驗證步驟清單

```bash
# 1. 確認 hidraw 設備存在
ls -la /dev/hidraw*
# 預期：出現 /dev/hidraw0 或類似

# 2. 確認 sysfs 中的 HID 設備
ls /sys/bus/hid/devices/
# 預期：列出 HID 設備目錄

# 3. 讀取 HID_ID 屬性（FWUPD 識別的關鍵）
cat /sys/bus/hid/devices/hid-*/hid_id
# 預期格式：0x06:0x04F3:0x3010（bus:vid:pid）
# 注意：bus=0x06 表示 BUS_VIRTUAL（uhid），bus=0x0F 表示 BUS_I2C（真實 I2C-HID）

# 4. 確認 i2c-hid 匯流排
ls /sys/bus/i2c-hid/
# 預期：列出 i2c-hid 設備

# 5. 使用 fwupdmgr 枚舉設備
fwupdmgr get-devices
# 預期：看見識別出的虛擬 HID 設備（如果 VID/PID 匹配）

# 6. 查看詳細設備資訊
fwupdmgr get-details
# 預期：顯示 VID/PID、設備名稱、instance ID

# 7. 如果有 Elantp 插件，檢查設備識別
fwupdtool --plugins elantp get-devices
```

### 7.3 預期的 FWUPD 輸出差異

| 項目 | 預期結果 |
|------|---------|
| `HID_ID` | `0x0F:04F3:3010`（BUS_I2C，如為虛擬則可能為 `0x06:xxxx:xxxx`） |
| `fwupdmgr get-devices` | 顯示設備（若 instance ID 匹配） |
| `/dev/hidrawX` | 存在 |
| `/sys/bus/i2c-hid/` | 有設備目錄 |

---

## 8. 自定義 QEMU I2C-HID 設備模型實作

### 8.1 實作策略

由於 QEMU 無原生 I2C-HID 設備，需開發 custom device model。實作方式：

**方式 A：基於 `smbus_slave.c` 擴展**
- QEMU 的 `SMBusSlaveInfo` 框架允許自定義 SMBus slave 設備
- 可在 `read_data` / `write_data` callback 中類比 HID-over-I2C 協定
- 實作較簡單，但不完全符合 I2C-HID 標準

**方式 B：從頭實作 I2C slave 設備**
- 使用 `I2CSlave` 框架（`i2c.h`）實作完整設備
- 類比 HID-over-I2C 的三個階段：
  1. **Power-on**：讀取 HID descriptor（from `hid-descr-addr` register）
  2. **Init**：讀取 Report descriptor
  3. **Operation**：處理 HID Input/Output reports
- 可完整欺騙 `i2c-hid` Linux 驅動程式

### 8.2 簡化版設備模型偽代碼（方式 B）

```c
// qemu/hw/i2c/virt-i2c-hid.c（概念性實作）

#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "hw/input/hid.h"

// I2C-HID 虛擬設備狀態
typedef struct VirtI2CHID {
    I2CSlave parent;
    
    // I2C-HID 狀態機
    uint8_t addr;           // 當前 I2C 地址
    uint8_t reg;            // 當前 register
    bool powered_on;
    
    // 模擬的 HID 描述元
    uint8_t hid_descriptor[64];
    uint8_t report_descriptor[256];
    
    // Elan-style 欺騙資料
    uint16_t vid;
    uint16_t pid;
    uint16_t module_id;
} VirtI2CHID;

// I2C-HID register 地址定義
#define HID_REG_HID_DESC        0x1D
#define HID_REG_REPORT_DESC     0x1E
#define HID_REG_REPORT_DATA     0x1F

static int virt_i2c_hid_recv(I2CSlave *s)
{
    VirtI2CHID *dev = VIRT_I2C_HID(s);
    
    // 根據當前 register 回應資料
    switch (dev->reg) {
    case HID_REG_HID_DESC:
        return dev->hid_descriptor[dev->addr++ & 0x3F];
    case HID_REG_REPORT_DESC:
        return dev->report_descriptor[dev->addr++ & 0xFF];
    default:
        return 0x00;
    }
}

static int virt_i2c_hid_send(I2CSlave *s, uint8_t data)
{
    VirtI2CHID *dev = VIRT_I2C_HID(s);
    
    // I2C-HID 命令處理
    // Command register format: [7:6] = operation, [5:0] = register/length
    uint8_t cmd = (data >> 6) & 0x03;
    uint8_t reg_or_len = data & 0x3F;
    
    switch (cmd) {
    case 0x00: // Set register
        dev->reg = reg_or_len;
        break;
    case 0x01: // Write report
        // 處理來自 host 的 reports
        break;
    case 0x02: // Read report
        dev->reg = reg_or_len;
        break;
    }
    return 0;
}

static void virt_i2c_hid_event(I2CSlave *s, enum I2CEvent event)
{
    VirtI2CHID *dev = VIRT_I2C_HID(s);
    
    if (event == I2C_START) {
        dev->powered_on = true;
    } else if (event == I2C_FINISH) {
        dev->powered_on = false;
    }
}

static void virt_i2c_hid_realize(DeviceState *dev, Error **errp)
{
    // 初始化虛擬 HID 描述元
    // ...
}

static void virt_i2c_hid_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);
    
    k->recv = virt_i2c_hid_recv;
    k->send = virt_i2c_hid_send;
    k->event = virt_i2c_hid_event;
    dc->realize = virt_i2c_hid_realize;
}

static const TypeInfo virt_i2c_hid_info = {
    .name = TYPE_VIRT_I2C_HID,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(VirtI2CHID),
    .class_init = virt_i2c_hid_class_init,
};

type_init(virt_i2c_hid_register_types)
```

### 8.3 QEMU 命令列掛載此設備

```bash
# 掛載自定義 I2C-HID 設備
qemu-system-aarch64 \
    -machine virt \
    -device virt-i2c-hid,id=vhid0,vid=0x04F3,pid=0x3010 \
    ...
```

---

## 9. 實作步驟規劃

### 9.1 分階段實作路線圖

```
Phase 1：環境建立
  ├── 確認 QEMU 安裝（aarch64 支援）
  ├── 準備 ARM Linux rootfs
  ├── 確認 Kernel 支援 i2c-hid
  └── 測試基本 ARM virt 啟動

Phase 2：DTB 注入
  ├── 建立 I2C bus DTB 節點
  ├── 確認 Linux i2c-dev 可見 I2C bus
  └── 確認 i2c-hid 驅動嘗試綁定

Phase 3：Custom QEMU Device
  ├── 基於 smbus_eeprom 或 i2c_slave 實作 VirtI2CHID
  ├── 實作 HID-over-I2C 協定（至少 HID descriptor）
  ├── 在 QEMU 中注册此設備
  └── 確認 VM 內核看到 i2c-hid 設備

Phase 4：FWUPD 整合
  ├── 確認 /dev/hidrawX 存在
  ├── 確認 sysfs HID_ID 正確
  ├── 使用 fwupdmgr get-devices 測試
  └── 測試 FWUPD update 流程（如有合適韌體）

Phase 5：強化與自動化
  ├── 實現完整的 HID Input/Output reports
  ├── 模擬 Elantp 插件的 Feature Report 流程
  └── 建立自動化測試腳本
```

### 9.2 各階段產出物

| 階段 | 主要產出 |
|------|---------|
| Phase 1 | 可啟動的 ARM virt VM + 確認 i2c-hid kernel module |
| Phase 2 | 自定義 DTB 檔案（`.dtb`）|
| Phase 3 | QEMU device model 程式碼 + 編譯過的 QEMU |
| Phase 4 | FWUPD 可識別虛擬設備 |
| Phase 5 | 端到端測試腳本 |

---

## 10. 預期限制與已知風險

### 10.1 根本限制

| 限制 | 說明 | 嚴重程度 |
|------|------|---------|
| **QEMU 無原生 I2C-HID** | 必須自行開發 QEMU device model | 🔴 高 |
| **HID-over-I2C 協定向量多** | 需完整實現才能騙過 i2c-hid 驅動 | 🔴 高 |
| **i2c-hid 驅動期望真實中斷** | QEMU 需能觸發 GPIO 中斷 | ⚠️ 中 |
| **HID Report Descriptor 複雜** | 需提供符合設備類型的正確 descriptor | ⚠️ 中 |

### 10.2 驗證 FWUPD 識別的限制

| 問題 | 說明 |
|------|------|
| **bus_type 可能不符預期** | 即使用 custom device，HID subsystem 的 bus_type 可能仍非 `BUS_I2C (0x0F)`，取決於 i2c-hid 核心驅動如何設定 |
| **Instance ID 不匹配** | FWUPD 需要 VID/PID 與 LVFS 上的韌體 metadata 匹配，虛擬設備的 VID/PID 可能沒有對應的韌體 |
| **Plugin 特定假設** | Elantp 插件有 IC type 檢查等特定假設，虛擬設備可能不通過 |

### 10.3 現實評估

根據可行性研究（`uhid_fwupd_feasibility.md`）的經驗教訓：

> **即使 QEMU + DTB + Custom Device 完整實現，FWUPD 的設備枚舉仍然可能失敗**，因為：
> 1. 設備的 `HID_ID` 可能不符合插件預期格式
> 2. FWUPD 需要與 LVFS 上的實際韌體 metadata 匹配
> 3. Plugin 的設備識別邏輯可能有硬體特定假設

**建議：將 Phase 3（Custom QEMU Device）的實作複雜度與預期收益對比考量**。若主要目標是測試 FWUPD 的設備發現流程，則 Phase 1-2 的 DTB 注入已足夠；若需完整集成測試，則需要 Phase 3。

---

## 11. 替代方案快速比較

| 方案 | 實現難度 | FWUPD 識別 | 韌體更新測試 | 適用場景 |
|------|---------|-----------|------------|---------|
| **uhid（用戶空間）** | 低 | ❌ 困難 | ⚠️ 有限 | 快速 HID 報告交換測試 |
| **fwupdtool --plugins** | 中 | N/A（離線） | ✅ 完全 | 插件邏輯測試 |
| **QEMU + DTB + Custom Device** | **極高** | ⚠️ 複雜 | ⚠️ 需韌體 | 完整系統集成測試 |
| **真實 HID over I2C 硬體** | N/A | ✅ 完整 | ✅ 完整 | 最終驗證 |
| **FWUPD Device Emulation** | 中 | ✅ 完整 | ✅ 完整 | **推薦：軟體模擬** |

### 11.1 推薦：FWUPD Device Emulation

FWUPD 2.x+ 支援軟體設備模擬（`fu_device_emulation_*` API），可用於在不需要真實或虛擬硬體的情況下測試整個 FWUPD 更新流程：

```bash
# 使用 fwupdtool 測試設備模擬
fwupdtool emulation start <device-guid> <device-file>
fwupdtool emulation attach <device-id>
# 然後執行一般的 fwupdmgr get-devices / update 等操作
```

此方法繞過了整個 HID 設備模擬的複雜性，直接測試 FWUPD 的設備發現、識別、更新流程。

---

## 12. 結論

### 12.1 可行性評估

**QEMU + 虛擬 I2C-HID DTB 方案在技術上可行，但實現成本極高。**

主要障礙：
1. **無 QEMU 原生支援**：必須自行實作完整的 QEMU I2C-HID 設備模型
2. **HID-over-I2C 協定複雜**：包含 Power-on sequence、HID descriptor讀取、Report descriptor讀取、Input/Output reports 等多個階段
3. **即便成功，仍可能無法騙過 FWUPD**：因為 FWUPD 的設備識別還需要正確的 VID/PID 與 LVFS metadata 匹配

### 12.2 建議

| 目標 | 推薦方案 |
|------|---------|
| 測試 FWUPD 設備發現流程 | **fwupdtool emulation**（繞過 HID 硬體模擬） |
| 測試 Plugin 邏輯（detach/attach） | **fwupdtool --plugins** |
| 驗證 FWUPD + I2C-HID 整合 | QEMU + DTB + **完整自定義 device model** |
| 最終驗證 | 真實 HID over I2C 硬體 |

### 12.3 如果選擇繼續 QEMU 方案

建議採用**漸進式實作**：
1. 先實現 Phase 1-2（DTB 注入），驗證 i2c-hid 驅動能否識別設備節點
2. 再評估 Phase 3 的必要性（如果 Phase 1-2 已經足夠滿足測試需求，可不繼續）
3. 如需完整集成，則實作 Custom QEMU Device Model（建議基於 `smbus_eeprom.c` 擴展，而非從頭實作）

---

## 參考資料

1. QEMU System ARM: virt board  
   https://www.qemu.org/docs/master/system/arm/virt.html

2. Linux Kernel: HID over I2C device tree binding  
   https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/hid/hid-over-i2c.txt

3. Linux Kernel: i2c-hid-core driver  
   https://github.com/torvalds/linux/blob/master/drivers/hid/i2c-hid/i2c-hid-core.c

4. QEMU I2C core: `hw/i2c/core.c`  
   https://github.com/qemu/qemu/blob/master/hw/i2c/core.c

5. QEMU SMBus slave framework: `hw/i2c/smbus_slave.c`  
   https://github.com/qemu/qemu/blob/master/hw/i2c/smbus_slave.c

6. QEMU SMBus EEPROM: `hw/i2c/smbus_eeprom.c`  
   https://github.com/qemu/qemu/blob/master/hw/i2c/smbus_eeprom.c

7. FWUPD HID Device Discovery  
   https://deepwiki.com/fwupd/fwupd/2.3-device-discovery

8. FWUPD Source: `libfwupdplugin/fu-hidraw-device.c`  
   https://github.com/fwupd/fwupd/blob/main/libfwupdplugin/fu-hidraw-device.c

9. 前導研究：uhid 虛擬 HID 裝置可行性  
   `D:\AiWorkSpace\M487_ScsiTool\doc\fwupd\uhid_fwupd_feasibility.md`
