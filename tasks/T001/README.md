# T001 — USB 複合裝置開發（MSC + HID I2C Bridge）

**狀態：** 🔧 實作中  
**起始日期：** 2026-03-26

---

## 目標

在 M487 上實作 USB 複合裝置（Composite Device）：
- **Interface 0：** HID I2C Bridge（Interrupt EP）
  - Report ID 0x01：I2C Write
  - Report ID 0x02：I2C Read
  - Report ID 0x03：I2C Write+Read
  - Report ID 0x04：I2C Scan
- **Interface 1：** USB Mass Storage（Bulk EP）
  - 30KB RAM Disk（60 sectors x 512 bytes）
  - BOT (Bulk-Only Transport) protocol

## USB 規格

| 項目 | 值 |
|------|-----|
| VID | 0x0416 |
| PID | 0x5020 |
| USB Speed | High-Speed (USB 2.0) |
| I2C Controller | UI2C0 |
| I2C Pins | PE2=CLK, PE3=DAT0 |
| I2C Speed | 100 kHz |

## 端點配置

| EP | 類型 | 方向 | 位址 | 最大封包大小 |
|----|------|------|------|-------------|
| EP0 | Control | IN/OUT | 0x00 | 64 |
| EP1 | Interrupt | IN | 0x81 | 512 (HS) / 64 (FS) |
| EP2 | Interrupt | OUT | 0x02 | 512 (HS) / 64 (FS) |
| EP3 | Bulk | IN | 0x83 | 512 |
| EP4 | Bulk | OUT | 0x04 | 512 |

## 韌體檔案

- `firmware/composite/main.c` — 主程式
- `firmware/composite/hid_i2c.c` — HID I2C + MSC 實作
- `firmware/composite/hid_i2c.h` — 定義
- `firmware/composite/usb_descriptors.c` — USB 描述符
- `firmware/composite/i2c_control.c` — I2C 控制驅動

## Branch

- `firmware/composite-rewrite`（Git branch）

## 待完成

- [ ] I2C Read 功能實作（目前 I2C_Read 函式未完成）
- [ ] 實體測試
- [ ] Windows C++ Win32 Tool 整合測試
