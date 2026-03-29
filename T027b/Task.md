# T027b - Firefly HID Filter Driver 範本研究

## 任務狀態: ✅ 完成

## 任務目標
研究 Microsoft Windows-driver-samples 中的 Firefly HID Filter Driver 範例，提取關鍵程式碼模式供 M487 HID Filter Driver 開發參考。

## 研究來源
- **Repo:** https://github.com/microsoft/Windows-driver-samples
- **分支:** main
- **路徑:** `hid/firefly/`
- **本地路徑:** `D:\AiWorkSpace\M487_ScsiTool\Windows-driver-samples\hid\firefly\`

## 研究結論

### 1. 架構概述
Firefly 是一個 **KMDF (Kernel Mode Driver Framework)** upper filter driver，專為 HID 滑鼠設計。
- 作為 `mouhid.sys` 的上層過濾器運行
- 目標設備：Microsoft USB Intellimouse Optical（多個 PID）
- 通訊介面：**WMI**（用於 user-mode ↔ kernel-mode 控制）

### 2. Upper Filter Attach/Detach 機制

**KMDF 方式（非常簡潔）：**
```c
// device.c - FireFlyEvtDeviceAdd()
WdfFdoInitSetFilter(DeviceInit);  // <-- 一行設定為 filter driver
```
KMDF 會自動處理所有 IRP 的 pass-through，無需手動設定 `IRP_MJ_XXX` dispatch table。

**INF 註冊方式：**
```inf
[Firefly_Inst_HWAddReg.NT]
HKR,,"UpperFilters",0x00010000,"Firefly"
```
在 `HID\Vid_045E&Pid_xxxx` 裝置的 registry 中加入 UpperFilters。

**設備名稱取得：**
```c
// 為了向 PDO 發送 IOCTL，需要先取得 PDO 名稱
WdfDeviceAllocAndQueryProperty(device, DevicePropertyPhysicalDeviceObjectName, ...);
// 存於 DeviceContext->PdoName (UNICODE_STRING)
```

### 3. IOCTL_HID_GET/SET_FEATURE 攔截模式

Firefly **不攔截**來自上層的 IOCTL，而是**主動向下層發送** IOCTL：

**核心模式（vfeature.c）：**
```c
// Step 1: 建立 I/O Target（連接至 PDO）
WdfIoTargetCreate(WdfObjectContextGetObject(DeviceContext), ...);
WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(&openParams, &DeviceContext->PdoName, FILE_WRITE_ACCESS);
WdfIoTargetOpen(hidTarget, &openParams);

// Step 2: 取得 Collection Information
WdfIoTargetSendIoctlSynchronously(hidTarget, NULL,
    IOCTL_HID_GET_COLLECTION_INFORMATION, NULL, &outputDescriptor, NULL, NULL);

// Step 3: 取得 Collection Descriptor (含 Preparsed Data)
WdfIoTargetSendIoctlSynchronously(hidTarget, NULL,
    IOCTL_HID_GET_COLLECTION_DESCRIPTOR, NULL, &outputDescriptor, NULL, NULL);

// Step 4: 分析 HID Capabilities
HidP_GetCaps(preparsedData, &caps);

// Step 5: 組建 Feature Report（使用 HIDP API）
HidP_SetUsages(HidP_Feature, PageId, 0, &usage, &usageLength,
    preparsedData, report, caps.FeatureReportByteLength);

// Step 6: 發送 Feature Report
WdfIoTargetSendIoctlSynchronously(hidTarget, NULL,
    IOCTL_HID_SET_FEATURE, &inputDescriptor, NULL, NULL);
```

**關鍵 IOCTL：**
| IOCTL | 用途 |
|-------|------|
| `IOCTL_HID_GET_COLLECTION_INFORMATION` | 取得 HID 集合基本資訊（包含 DescriptorSize） |
| `IOCTL_HID_GET_COLLECTION_DESCRIPTOR` | 取得完整的 HID Report Descriptor |
| `IOCTL_HID_SET_FEATURE` | 發送 Feature Report 給設備 |
| `IOCTL_HID_GET_FEATURE` | 讀取設備的 Feature Report |

### 4. IRP Pass-Through 處理

KMDF filter driver 的 IRP pass-through 是**全自動**的：
- `WdfFdoInitSetFilter()` 告知 framework 這是 filter driver
- Framework 自動將所有 IRP 傳遞給下層設備
- 若要自訂處理某個 IRP，使用 `WDF_IO_FORWARD_COMMON_CREATE_CONFIG` 或設定 `EvtDeviceFilterRemoveResourceRequirements`

**不需要**像 WDM 一樣手動實作 `IRP_MJ_XXX` dispatch function 並呼叫 `IoCompleteRequest`。

### 5. WMI 介面（User-Mode 控制通道）

**MOF 定義（firefly.mof）：**
```mof
class FireflyDeviceInformation
{
    [key, read] string InstanceName;
    [read] boolean Active;
    [WmiDataId(1), read, write] boolean TailLit;
};
```

**Driver 端（WMI 事件處理）：**
```c
// WMI 初始化
WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &FireflyDeviceInformation_GUID);
WdfWmiInstanceCreate(Device, &instanceConfig, &woa, &instance);

// Set 請求處理（user-mode 寫入 TailLit）
EvtWmiInstanceSetInstance() → FireflySetFeature() → IOCTL_HID_SET_FEATURE

// Query 請求處理（user-mode 讀取 TailLit）  
EvtWmiInstanceQueryInstance() → 回傳當前 TailLit 狀態
```

**User-Mode 應用程式（flicker.exe）：**
- 透過 COM/WMI API 連接 `root\WMI` namespace
- 開啟 `FireflyDeviceInformation` class 的 instance
- 呼叫 `IWbemServices::PutInstance` 寫入 `TailLit` 屬性
- driver 的 `EvtWmiInstanceSetInstance` 被觸發

### 6. 關鍵資料結構

**DEVICE_CONTEXT：**
```c
typedef struct _DEVICE_CONTEXT {
    FireflyDeviceInformation WmiInstance;  // WMI 資料
    UNICODE_STRING PdoName;                // PDO 裝置名稱（用於 IO Target）
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;
```

**HID Feature 常數（magic.h）：**
```c
#define TAILLIGHT_PAGE     0xFF   // Vendor-defined usage page
#define TAILLIGHT_FEATURE  0x02   // Tail light feature usage ID
```

### 7. 與 M487 HID Filter Driver 的相關性

| Firefly 模式 | M487 可借鑒之處 |
|-------------|----------------|
| `WdfFdoInitSetFilter()` | 註冊為 HID upper filter |
| `WdfIoTargetOpen(PDO name)` | 開啟下層 HID 裝置通訊 |
| `IOCTL_HID_SET_FEATURE` | 控制 HID 裝置的 feature report |
| `HidP_SetUsages()` / `HidP_GetUsages()` | HID Report 解析/修改 |
| WMI class registration | User-mode ↔ Kernel-mode 通訊 |
| UpperFilters registry | 安裝為 upper filter |

### 8. 重要發現

1. **Firefly 不是 IRP 攔截器**：它不攔截经过的 IRP，而是主動向下層設備發送 IOCTL。
2. **KMDF 簡化了 filter driver**：不需要 IRP dispatch table，framework 自動處理 pass-through。
3. **PDO 名稱是關鍵**：要向下層發送 IOCTL，必須先取得 PDO 的名稱（`\Device\USB...`之類）。
4. **WMI 是常見的 user-mode 控制介面**：適用於需要非同步控制訊號的場景。
5. **HIDP API 是關鍵**：使用 `HidP_GetCaps`、`HidP_SetUsages`、`HidP_GetUsages` 來操作 HID report。

## 輸出檔案
- 研究報告本檔案（Task.md）
- `D:\AiWorkSpace\M487_ScsiTool\Windows-driver-samples\hid\firefly\`（完整原始碼）
- `D:\AiWorkSpace\M487_ScsiTool\T027b\Firefly_Research_Report.md`（獨立研究報告）

## 完成時間
2026-03-28 12:14 GMT+8
