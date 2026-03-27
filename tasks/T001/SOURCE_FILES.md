# T001 — 原始碼檔案

## 韌體位置

所有原始碼位於：
`D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`

| 檔案 | 說明 |
|------|------|
| `main.c` | 主程式，系統初始化 |
| `hid_i2c.c` | HID MSC 實作（~600行）|
| `hid_i2c.h` | 定義與結構 |
| `usb_descriptors.c` | USB 描述符 |
| `i2c_control.c` | I2C UI2C0 驅動（~300行）|

## 編譯產物

- `build_gcc/firmware.elf`
- `build_gcc/firmware.bin`（燒錄用）
- `build_gcc/firmware.hex`

## Windows Tool

位於：
`D:\AiWorkSpace\KM\M487-Examples\USB_HS_Samples\HSUSBD_HID_Transfer_And_MSC\WindowsTool\`

可用於測試 HID 界面。
