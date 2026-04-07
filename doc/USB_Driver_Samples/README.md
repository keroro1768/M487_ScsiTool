# USB 驅動程式範例知識庫

## 📋 概述

本知識庫是基於 Microsoft Windows-driver-samples 專案，對 USB 驅動程式範例進行深入分析的結果。主要目的是為 M487_ScsiTool USB Filter Driver 專案（T027）提供技術參考。

## 📁 目錄結構

```
USB_Driver_Samples/
├── README.md                    # 本檔案 - 索引與概述
├── usb/                         # USB 通用範例分析
│   ├── README.md               # USB 範例總覽
│   ├── usbsamp_Analysis.md     # usbsamp 通用 USB 裝置驅動程式分析
│   └── kmdf_fx2_Analysis.md    # kmdf_fx2 KMDF USB 裝置分析
├── hid/                         # HID 範例分析
│   ├── README.md               # HID 範例總覽
│   ├── firefly_Deep_Dive.md    # 🔥 firefly 深入分析（最關鍵！）
│   └── hid_Architecture.md     # Windows HID 堆疊架構
├── filter/                      # Filter 驅動程式分析
│   ├── README.md               # USB Filter Driver 分析總覽
│   └── Filter_Driver_Pattern.md # 常見 Filter 驅動程式模式
├── Architecture/                # 核心架構分析
│   ├── USB_Descriptor_Analysis.md  # USB 描述符分析
│   ├── URB_Handling_Patterns.md    # URB 生命週期與處理
│   └── IRP_Stack_Deep_Dive.md     # IRP_MJ_INTERNAL_DEVICE_CONTROL 深入解析
└── M487/                       # M487 專用設計文件
    └── M487_Filter_Driver_Design.md # 如何借鑒 firefly 設計 M487 Filter Driver
```

## 🔑 核心範例

### 1. firefly（HID Upper Filter Driver）⭐ 最關鍵

**位置：** `hid/firefly/`

這是 KMDF HID 上層過濾驅動程式的範例，與 M487 USB Filter Driver 需求最為匹配。

**關鍵功能：**
- 作為 HID 裝置的上層過濾器附加至裝置堆疊
- 使用 WMI 介面與使用者模式應用程式通訊
- 透過 IO Target 發送 HID IOCTL 控制報告
- 攔截和處理 Feature Report

**學習重點：**
- INF 檔案的 UpperFilters 註冊方式
- 如何開啟 PDO 並發送 IOCTL
- HidP_GetCaps、HidP_SetUsages 的使用

### 2. usbsamp（通用 USB 裝置驅動程式）

**位置：** `usb/usbsamp/`

學習 USB 描述符處理、URB 操作的完整範例。

### 3. kmdf_fx2（KMDF USB 裝置驅動程式）

**位置：** `usb/kmdf_fx2/`

OSR USB-FX2 開發板的驅動程式，展示：
- USB 裝置初始化與描述符讀取
- Bulk/Interrupt 傳輸處理
- IOCTL 介面設計

### 4. toaster filter（通用 Filter Driver）

**位置：** `general/toaster/toastDrv/kmdf/filter/generic/`

學習 KMDF Filter Driver 的基本模式：
- WdfFdoInitSetFilter 的使用
- 請求轉發機制
- 完成例程的處理

## 📚 分析重點

### USB 描述符處理
- 裝置描述符（Device Descriptor）
- 設定描述符（Configuration Descriptor）
- 介面描述符（Interface Descriptor）
- 端點描述符（Endpoint Descriptor）
- HID 描述符（HID Descriptor、Report Descriptor）

### URB 處理模式
- URB 請求的建立與傳送
- 同步 vs 非同步傳輸
- 傳輸類型：Control、Bulk、Interrupt、Isochronous

### IRP 堆疊操作
- IRP_MJ_INTERNAL_DEVICE_CONTROL 的處理
- IOCTL 的定義與使用
- WDF I/O Target 的使用

### Filter Driver 模式
- 上層過濾器（Upper Filter）vs 下層過濾器（Lower Filter）
- 請求攔截與轉發
- 橋接至下層裝置堆疊

## 🎯 M487 專案適用性

| 範例 | 適用性 | 原因 |
|------|--------|------|
| firefly | ⭐⭐⭐⭐⭐ | HID Filter Driver 完整實作，INF 註冊方式可直接借鑒 |
| kmdf_fx2 | ⭐⭐⭐⭐ | USB 裝置操作模式詳細，IOCTL 設計可參考 |
| usbsamp | ⭐⭐⭐ | URB 處理全面，但較為複雜 |
| toaster filter | ⭐⭐⭐ | Filter Driver 基本模式 |

## 🔗 相關資源

- [Microsoft Windows-driver-samples](https://github.com/microsoft/Windows-driver-samples)
- [Windows HID 架構](https://docs.microsoft.com/windows-hardware/drivers/hid/)
- [KMDF 文件](https://docs.microsoft.com/windows-hardware/drivers/wdf/)
- [USB 驅動程式開發](https://docs.microsoft.com/windows-hardware/drivers/usbcon/)

## 📝 使用說明

1. **新手起點：** 先閱讀 `hid/hid_Architecture.md` 了解 HID 堆疊
2. **核心學習：** 仔細研讀 `hid/firefly_Deep_Dive.md`
3. **設計參考：** 閱讀 `M487/M487_Filter_Driver_Design.md` 了解如何應用於 M487
4. **深入研究：** 根據需要閱讀 `Architecture/` 目錄下的技術文件

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
