# Microsoft HID over I2C 介紹

> Microsoft HID over I2C Protocol Specification
> 版本：1.0
> 官網下載：https://download.microsoft.com/download/7/D/D/7DD44BB7-2A7A-4505-AC1C-7227D3D96D5B/hid-over-i2c-protocol-spec-v1-0.docx

---

## 什麼是 HID over I2C？

HID over I2C (HID-I2C) 是微軟定義的一種讓簡單輸入裝置透過 I2C 匯流排與 Windows 通訊的協定。

### 設計目的

| 目標 | 說明 |
|------|------|
| **低功耗** | 適合行動裝置 |
| **簡單匯流排** | 使用 I2C 而非 USB |
| **標準化** | 統一的 Windows 驅動程式 |

---

## 系統架構

```
┌─────────────────────────────────────────────────────────────┐
│                      Windows OS                            │
│  ┌──────────────┐    ┌──────────────┐                    │
│  │  HID Class  │◄───│  I2C Miniport│                    │
│  │  Driver     │    │  Driver      │                    │
│  └──────────────┘    └──────────────┘                    │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    I2C 裝置                               │
│  ┌──────────────┐    ┌──────────────┐                    │
│  │  HID I2C    │◄───│  感測器/按鍵 │                    │
│  │  Controller  │    │              │                    │
│  └──────────────┘    └──────────────┘                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 通訊協定

### I2C 位址

- **預設位址**：0x2A (可設定)
- **格式**：7-bit I2C 位址

### 資料傳輸

| 類型 | 大小 | 說明 |
|------|------|------|
| Input Report | 可變 | 裝置 → 主機 |
| Output Report | 可變 | 主機 → 裝置 |
| Feature Report | 可變 | 設定/配置 |

---

## 與 USB HID 的比較

| 特性 | USB HID | HID over I2C |
|------|---------|---------------|
| 速度 | 12-480 Mbps | 400 Kbps (Fast I2C) |
| 功耗 | 較高 | 較低 |
| 驅動程式 | 內建 | 內建 (Windows 8+) |
| 複雜度 | 中等 | 簡單 |

---

## Windows 支援

| Windows 版本 | 支援 |
|-------------|------|
| Windows 8+ | ✅ 內建 |
| Windows 7 | ❌ 不支援 |

---

## 應用場景

| 應用 | 範例 |
|------|------|
| 觸控板 | Laptop touchpad |
| 鍵盤 | 鍵盤矩陣 |
| 遙控器 | 紅外線遙控 |
| 感測器 | 環境光感測 |

---

## 實際產品

| 晶片 | 說明 |
|------|------|
| FT260 | FTDI HID over I2C 晶片 |
| GD32F350 | 晶心科技 |

---

## 參考資源

| 資源 | 連結 |
|------|------|
| 官方規範 (DOCX) | [下載](https://download.microsoft.com/download/7/D/D/7DD44BB7-2A7A-4505-AC1C-7227D3D96D5B/hid-over-i2c-protocol-spec-v1-0.docx) |
| Microsoft Learn | [Introduction to HID Over I2C](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/hid-over-i2c-guide) |
| 架構概述 | [Architecture and Overview](https://learn.microsoft.com/en-us/windows-hardware/drivers/hid/architecture-and-overview) |
| Linux 支援 | [i2c-hid driver](https://www.kernel.org/doc/Documentation/devicetree/bindings/input/hid-over-i2c.txt) |

---

## 重點摘要

1. **用途**：讓 I2C 裝置被 Windows 識別為 HID 裝置
2. **優點**：免驅動、內建支援、低功耗
3. **版本**：v1.0 (2012年發布)
4. **下載**：Microsoft 官網提供 DOCX 格式

---

*最後更新：2026-03-21*
