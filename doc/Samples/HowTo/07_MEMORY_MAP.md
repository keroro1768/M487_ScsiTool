# M487 記憶體對照表

## 📊 M487JIDAE 記憶體

| 類型 | 大小 | 起始位址 | 說明 |
|------|------|-----------|------|
| App Flash (APROM) | 512KB | 0x0000_0000 | 應用程式儲存 |
| SRAM (APRAM) | 160KB | 0x2000_0000 | 系統記憶體 |
| Flash Config | 4KB | 0x0030_0000 | 晶片設定值 |

---

## 📀 Flash 組織（M487JIDAE）

| 區域 | 大小 | 位址範圍 |
|------|------|----------|
| APROM | 512KB | 0x0000_0000 ~ 0x0007_FFFF |
| CONFIG | 4KB | 0x0030_0000 ~ 0x0030_0FFF |
| LDROM | — | （M487JIDAE 無 LDROM） |

---

## 💾 USB MSC RAM Disk 配置

```c
/* MSC Storage Disk：1MB RAM 磁碟 */
#define STORAGE_DISK_SIZE   (2048 * 512)  // 1,048,576 bytes
static uint8_t g_au8StorageDisk[STORAGE_DISK_SIZE];
```

**記憶體位置**：SRAM 區，與 USB Descriptor、變數共存

> ⚠️ M487 SRAM 僅 160KB，1MB 會超出。實際使用時需調整 STORAGE_DISK_SIZE 至 160KB 以內。

---

## 🔧 SWD 調試資訊

| 項目 | 數值 |
|------|------|
| SWD IDCODE | 0x2BA01477 |
| Target | NuMicro.cpu (Cortex-M4) |
| Breakpoints | 6 |
| Watchpoints | 4 |
| 預設 SWD CLK | 1000 kHz |
| Transport | hla_swd |

---

## 📍 GCC Linker Script 記憶體配置

```
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
    RAM (rwx)   : ORIGIN = 0x20000000, LENGTH = 160K
}
```

---

## 🔌 USB Descriptor VID/PID

| 裝置 | VID | PID | 說明 |
|------|-----|-----|------|
| Nu-Link | 0x0416 | 0x511C | 調試器 |
| VENDOR_LBK | 0x0416 | 0xFF20 | USB Vendor Loopback |
| MSC (Default) | 0x0416 | 0xFF20 | USB Mass Storage |

---

## 🗺️  Vector Table（M487 起始）

| 位址 |內容 | 說明 |
|------|-----|------|
| 0x0000_0000 | 0x20014520 | Initial SP (Stack Pointer) |
| 0x0000_0004 | 0x0000_02E5 | Reset_Handler (PC) |

驗證：`xPSR: 0x01000000 pc: 0x000002E4 msp: 0x20014520`

---

## 📊 Flash 讀取速度

| 讀取大小 | 耗時 | 速度 |
|----------|------|------|
| 4KB | ~1.3s | 3.1 KiB/s |
| 64KB | ~19s | 3.3 KiB/s |
| 512KB | ~158s | 3.2 KiB/s |

速度瓶頸：SWD 半雙工協定的 overhead
