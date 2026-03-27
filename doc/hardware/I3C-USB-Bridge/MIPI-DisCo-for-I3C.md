# MIPI DisCo for I3C 介紹

> MIPI DisCo (Discovery and Configuration) for I3C - OS 識別框架
> 版本：v1.1 (2023年2月發布)
> 官方下載：https://www.mipi.org/mipi-disco-for-i3c-download

---

## 什麼是 MIPI DisCo for I3C？

MIPI DisCo for I3C 是一個**作業系統識別框架**，用於簡化 I3C 感測器的軟體整合。

### 核心功能

| 功能 | 說明 |
|------|------|
| **OS 識別** | 讓 OS 自動識別 MIPI I3C 裝置 |
| **驅動自動載入** | 自動載入對應的驅動程式 |
| **標準化描述** | 統一的裝置描述格式 |

### 支援的作業系統

- Android
- macOS X
- Microsoft Windows
- Linux

---

## 運作原理

```
┌─────────────────────────────────────────────────────────────┐
│                    主機系統 (OS)                            │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐    ┌──────────────┐    ┌────────────┐  │
│  │   OS Kernel  │◄───│  DisCo      │◄───│   Driver   │  │
│  │              │    │  Parser     │    │   Loader   │  │
│  └──────────────┘    └──────────────┘    └────────────┘  │
│         │                   │                  │           │
└─────────┼───────────────────┼──────────────────┼───────────┘
          │                   │                  │
          ▼                   ▼                  ▼
┌─────────────────────────────────────────────────────────────┐
│                    I3C 裝置                                │
│  ┌──────────────┐    ┌──────────────┐                    │
│  │  DisCo XML   │    │  I3C Sensor  │                    │
│  │  (描述檔)    │    │              │                    │
│  └──────────────┘    └──────────────┘                    │
└─────────────────────────────────────────────────────────────┘
```

---

## 與 I3C 生態系的關係

| 規格 | 角色 |
|------|------|
| **MIPI I3C** | 實體層與通訊協定 |
| **MIPI I3C Basic** | I3C 子集（免授權費） |
| **MIPI I3C HCI** | 主機控制器介面 |
| **MIPI DisCo for I3C** | OS 識別與組態配置 |

---

## 為什麼需要 DisCo？

### 傳統問題
```
I2C 裝置 → 每個廠商需要寫專屬驅動 → 昂貴且耗時
```

### DisCo 解決方案
```
I3C 裝置 + DisCo 描述檔 → OS 自動識別 → 免驅動開發
```

---

## DisCo 描述檔格式

範例結構：
```xml
<?xml version="1.0" encoding="utf-8"?>
<CompatibleDevice>
    <DeviceId>
        <Manufacturer>ACME Corp</Manufacturer>
        <ProductID>0x1234</ProductID>
    </DeviceId>
    <Resources>
        <I3C>
            <StaticAddress>0x68</StaticAddress>
            <BusFrequency>12000000</BusFrequency>
        </I3C>
    </Resources>
</CompatibleDevice>
```

---

## 對 USB to I3C Bridge 的意義

| 應用 | 說明 |
|------|------|
| **橋接器識別** | DisCo 可用於識別 USB I3C Bridge 裝置 |
| **熱插拔支援** | OS 可自動偵測 I3C 裝置插拔 |
| **統一介面** | 標準化 I3C 裝置描述，簡化開發 |

---

## 相關資源

| 資源 | 連結 |
|------|------|
| 官方下載 | https://www.mipi.org/mipi-disco-for-i3c-download |
| 規格說明 | https://www.mipi.org/specifications/mipi-disco-i3c |
| I3C 主規格 | https://www.mipi.org/specifications/i3c-sensor-specification |
| I3C HCI | https://www.mipi.org/mipi-i3c-hci-download |

---

## 重點摘要

1. **用途**：讓作業系統自動識別 I3C 裝置
2. **優點**：免驅動程式開發，快速整合
3. **相容性**：支援 Android、Windows、Linux、macOS
4. **版本**：v1.1 (2023年2月)
5. **免費下載**：MIPI 官網提供公開下載

---

*最後更新：2026-03-21*
