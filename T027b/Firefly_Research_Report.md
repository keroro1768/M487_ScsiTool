# Firefly HID Filter Driver 研究報告

> **來源：** Microsoft Windows-driver-samples / `hid/firefly/`
> **用途：** KMDF HID Upper Filter Driver 範例，攔截並控制 Microsoft Optical Mouse 的 LED
> **本地路徑：** `D:\AiWorkSpace\M487_ScsiTool\Windows-driver-samples\hid\firefly\`

---

## 1. 架構總覽

```
┌─────────────────────────────────────┐
│  User-Mode Application (flicker.exe) │  ← WMI COM API
└──────────────┬──────────────────────┘
               │ WMI (root\WMI)
┌──────────────▼──────────────────────┐
│  Firefly.sys (KMDF Upper Filter)     │  ← FireFlyEvtDeviceAdd + WMI
└──────────────┬──────────────────────┘
               │ IOCTL_HID_SET_FEATURE + IOCTL_HID_GET_*
┌──────────────▼──────────────────────┐
│  mouhid.sys (HID Minidriver)        │
└──────────────┬──────────────────────┘
               │ USB HRs
┌──────────────▼──────────────────────┐
│  usbd.sys + USB Hardware            │
└─────────────────────────────────────┘
```

Firefly 是 **KMDF upper filter driver**，掛載在 `mouhid.sys` 上方，用於控制滑鼠 LED。

---

## 2. Upper Filter Attach/Detach 機制

### 2.1 KMDF 方式（核心）

```c
// device.c - FireFlyEvtDeviceAdd()
NTSTATUS
FireFlyEvtDeviceAdd(WDFDRIVER Driver, PWDFDEVICE_INIT DeviceInit)
{
    // ...前期初始化...
    
    // ★ 關鍵：設定為 filter driver（upper 位置）
    WdfFdoInitSetFilter(DeviceInit);
    
    // 創建 WDFDEVICE
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    
    // 初始化 WMI
    WmiInitialize(device, pDeviceContext);
    
    // 取得 PDO 名稱（用於後續 IO Target）
    WdfDeviceAllocAndQueryProperty(device,
        DevicePropertyPhysicalDeviceObjectName,
        NonPagedPoolNx, &attributes, &memory);
    
    return status;
}
```

**原理：** `WdfFdoInitSetFilter()` 告訴 KMDF 這是一個 filter device，framework 會自動將所有 PnP/Power IRP 傳遞給下層，無需手動設定 IRP dispatch table。

### 2.2 INF 註冊方式

```inf
; firefly.inx

[Firefly_Inst_HWAddReg.NT]
; 在 HID 裝置的 UpperFilters 加入 "Firefly"
HKR,,"UpperFilters",0x00010000,"Firefly"
```

目標 HID 硬體 ID：
- `HID\Vid_045E&Pid_001E` (Microsoft IntelliMouse Optical)
- `HID\Vid_045E&Pid_0029` (IntelliMouse Explorer 1.1A)
- `HID\Vid_045E&Pid_0039` (IntelliMouse Explorer 3.0)
- `HID\Vid_045E&Pid_0040` (IntelliMouse Explorer 4.0)
- `HID\Vid_045E&Pid_0047` (Wireless IntelliMouse Explorer)

### 2.3 PDO 名稱取得

要向下層設備發送 IOCTL，需要先開啟 PDO。KMDF 提供：
```c
// 取得 PDO 的裝置名稱（UNC）
WdfDeviceAllocAndQueryProperty(device,
    DevicePropertyPhysicalDeviceObjectName,  // 取得 \Device\USB...
    NonPagedPoolNx,
    &attributes,
    &memory);

pDeviceContext->PdoName.Buffer = WdfMemoryGetBuffer(memory, &bufferLength);
pDeviceContext->PdoName.MaximumLength = (USHORT)bufferLength;
pDeviceContext->PdoName.Length = (USHORT)(bufferLength - sizeof(UNICODE_NULL));
```

---

## 3. IOCTL_HID_GET/SET_FEATURE 攔截與發送

### 3.1 設計理念

Firefly **不是被動攔截** IRP，而是**主動發送** IOCTL 到下層設備。當 user-mode 透過 WMI 請求控制 LED 時，driver 才會向 HID 設備發送 IOCTL。

### 3.2 完整流程（vfeature.c）

```c
NTSTATUS
FireflySetFeature(
    PDEVICE_CONTEXT DeviceContext,
    UCHAR           PageId,        // 0xFF (vendor-defined)
    USHORT          FeatureId,     // 0x02 (tail light)
    BOOLEAN         EnableFeature  // TRUE=on, FALSE=off
)
{
    WDFIOTARGET hidTarget;
    
    // Step 1: 建立 I/O Target
    WdfIoTargetCreate(WdfObjectContextGetObject(DeviceContext),
        WDF_NO_OBJECT_ATTRIBUTES, &hidTarget);
    
    // Step 2: 開啟 PDO（以 Write Access）
    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        &DeviceContext->PdoName,
        FILE_WRITE_ACCESS);
    openParams.ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_READ;
    WdfIoTargetOpen(hidTarget, &openParams);
    
    // Step 3: 取得 Collection Information
    HID_COLLECTION_INFORMATION CollectionInfo = {0};
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor,
        &CollectionInfo, sizeof(CollectionInfo));
    WdfIoTargetSendIoctlSynchronously(hidTarget, NULL,
        IOCTL_HID_GET_COLLECTION_INFORMATION,
        NULL, &outputDescriptor, NULL, NULL);
    // CollectionInfo.DescriptorSize 告訴我們需要多大的 buffer
    
    // Step 4: 取得 Collection Descriptor（含 HID Report Descriptor + Preparsed Data）
    PHIDP_PREPARSED_DATA preparsedData;
    preparsedData = ExAllocatePool2(POOL_FLAG_NON_PAGED,
        CollectionInfo.DescriptorSize, 'ffly');
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&outputDescriptor,
        preparsedData, CollectionInfo.DescriptorSize);
    WdfIoTargetSendIoctlSynchronously(hidTarget, NULL,
        IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
        NULL, &outputDescriptor, NULL, NULL);
    
    // Step 5: 分析 HID Capabilities
    HIDP_CAPS caps;
    RtlZeroMemory(&caps, sizeof(HIDP_CAPS));
    HidP_GetCaps(preparsedData, &caps);
    // caps.FeatureReportByteLength → Feature report 的長度
    
    // Step 6: 建立 Feature Report
    PCHAR report = ExAllocatePool2(POOL_FLAG_NON_PAGED,
        caps.FeatureReportByteLength, 'ffly');
    
    if (EnableFeature) {
        USAGE usage = FeatureId;  // 0x02
        ULONG usageLength = 1;
        HidP_SetUsages(HidP_Feature, PageId, 0,
            &usage, &usageLength,
            preparsedData, report,
            caps.FeatureReportByteLength);
    }
    
    // Step 7: 發送 Feature Report
    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputDescriptor,
        report, caps.FeatureReportByteLength);
    WdfIoTargetSendIoctlSynchronously(hidTarget, NULL,
        IOCTL_HID_SET_FEATURE,
        &inputDescriptor, NULL, NULL, NULL);
    
    // cleanup...
}
```

### 3.3 關鍵 IOCTL 說明

| IOCTL | 方向 | 用途 |
|-------|------|------|
| `IOCTL_HID_GET_COLLECTION_INFORMATION` | Driver → HID Class | 取得 HID 集合基本資訊（回傳 `HID_COLLECTION_INFORMATION`） |
| `IOCTL_HID_GET_COLLECTION_DESCRIPTOR` | Driver → HID Class | 取得完整的 HID Descriptor（Report Descriptor + Preparsed Data） |
| `IOCTL_HID_SET_FEATURE` | Driver → HID Class | 發送 Feature Report 給底層設備 |
| `IOCTL_HID_GET_FEATURE` | Driver → HID Class | 讀取設備的 Feature Report |

### 3.4 HIDP API 使用

```c
// 設定 Usage（在 Feature Report 中標記某個 feature 為 active）
HidP_SetUsages(
    HidP_Feature,        // Report type
    PageId,             // Usage page (0xFF = vendor)
    0,                  // Link collection (Reserved)
    &usage,             // Usage ID list
    &usageLength,       // Number of usages
    preparsedData,      // Preparsed data from device
    report,             // Report buffer
    reportLength        // Report buffer length
);

// 讀取 Usage
HidP_GetUsages(HidP_Feature, PageId, 0, &usage, &usageLength,
    preparsedData, report, reportLength);

// 取得設備 capabilities
HidP_GetCaps(preparsedData, &caps);
// caps.FeatureReportByteLength, caps.InputReportByteLength, etc.
```

---

## 4. IRP Pass-Through 處理

### 4.1 KMDF Filter Driver 的自動 Pass-Through

在 KMDF 中，使用 `WdfFdoInitSetFilter()` 註冊後：
- **所有 PnP IRP** 自動 pass-through 到下層
- **所有 Power IRP** 自動 pass-through 到下層
- **所有 I/O IRP** 自動 pass-through 到下層

不需要像 WDM 那樣實作：
```c
// WDM 傳統方式（KMDF 不需要）
DRIVER_DISPATCH FireflyDispatch;
case IRP_MJ_PNP: ...
case IRP_MJ_POWER: ...
```

### 4.2 若要自訂 IRP 處理

KMDF 允許覆寫特定的 I/O 事件：
```c
// 覆寫 Create 請求
WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
attributes.EventHandler = EvtDeviceFileCreate;
attributes.Context = &device;
WdfDeviceInitSetFileObjectConfig(DeviceInit,
    &fileConfig, &attributes);

// 覆寫 IOCTL 請求
WDF_IO_QUEUE_CONFIG_INIT(&cfg, WdfIoQueueDispatchParallel);
cfg.EvtIoDeviceControl = EvtDeviceIoDeviceControl;
WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);
```

---

## 5. WMI 介面（User-Mode 控制通道）

### 5.1 MOF 定義（firefly.mof）

```mof
[Dynamic, Provider("WMIProv"),
 WMI,
 Description("Firefly driver information"),
 guid("{AB27DB29-DB25-42E6-A3E7-28BD46BDB666}"),
 locale("MS\\0x409")]
class FireflyDeviceInformation
{
    [key, read] string InstanceName;
    [read] boolean Active;
    [WmiDataId(1), read, write] boolean TailLit;
};
```

### 5.2 Driver 端 WMI 初始化（wmi.c）

```c
NTSTATUS WmiInitialize(WDFDEVICE Device, PDEVICE_CONTEXT DeviceContext)
{
    // 設定 MOF resource 名稱
    WdfDeviceAssignMofResourceName(Device, &mofRsrcName);
    
    // 設定 WMI Provider
    WDF_WMI_PROVIDER_CONFIG_INIT(&providerConfig, &FireflyDeviceInformation_GUID);
    providerConfig.MinInstanceBufferSize = sizeof(FireflyDeviceInformation);
    
    // 設定 WMI Instance
    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&instanceConfig, &providerConfig);
    instanceConfig.Register = TRUE;
    instanceConfig.EvtWmiInstanceQueryInstance = EvtWmiInstanceQueryInstance;
    instanceConfig.EvtWmiInstanceSetInstance = EvtWmiInstanceSetInstance;
    instanceConfig.EvtWmiInstanceSetItem = EvtWmiInstanceSetItem;
    
    // 建立 WMI Instance
    WdfWmiInstanceCreate(Device, &instanceConfig, &woa, &instance);
}
```

### 5.3 Set 請求處理

```c
NTSTATUS EvtWmiInstanceSetInstance(
    WDFWMIINSTANCE WmiInstance,
    ULONG InBufferSize,
    PVOID InBuffer)
{
    FireflyDeviceInformation* pInfo = InstanceGetInfo(WmiInstance);
    
    // 複製 user-mode 傳來的資料
    RtlMoveMemory(pInfo, InBuffer, sizeof(*pInfo));
    
    // ★ 關鍵：將 TailLit 狀態寫入 HID 設備
    status = FireflySetFeature(
        WdfObjectGet_DEVICE_CONTEXT(WdfWmiInstanceGetDevice(WmiInstance)),
        TAILLIGHT_PAGE,      // 0xFF
        TAILLIGHT_FEATURE,    // 0x02
        pInfo->TailLit        // TRUE/FALSE
    );
    
    return status;
}
```

### 5.4 User-Mode 應用程式（flicker.exe）

```cpp
// 連接 WMI
IWbemServices* pSvc = ConnectToNamespace(NAME_SPACE); // "root\\WMI"

// 取得 instance
IWbemClassObject* pObj = GetInstanceReference(pSvc, CLASS_NAME);
// CLASS_NAME = "FireflyDeviceInformation"

// 設定 TailLit 屬性
VARIANT vtProp;
vtProp.vt = VT_BOOL;
vtProp.boolVal = bEnable ? VARIANT_TRUE : VARIANT_FALSE;
pSvc->PutInstance(pObj, ...);

// 或使用 SetItem（更高效）
pSvc->Set(pObj, PROPERTY_NAME, ...);  // PROPERTY_NAME = "TailLit"
```

---

## 6. 關鍵檔案對照表

| 檔案 | 功能 |
|------|------|
| `driver/driver.c` | DriverEntry + WDF_DRIVER_CONFIG_INIT |
| `driver/device.c` | FireFlyEvtDeviceAdd + PDO 名稱取得 |
| `driver/device.h` | DEVICE_CONTEXT 結構定義 |
| `driver/firefly.h` | 主要 header，包含所有 module headers |
| `driver/vfeature.c` | IOCTL_HID_SET/GET_FEATURE 發送邏輯 |
| `driver/vfeature.h` | FireflySetFeature 宣告 |
| `driver/wmi.c` | WMI 初始化 + Set/Query/SetItem handlers |
| `driver/wmi.h` | WMI 函式宣告 |
| `driver/firefly.mof` | WMI class 定義（MOF 格式） |
| `driver/firefly.inx` | INF 安裝檔（UpperFilters 設定） |
| `driver/magic.h` | TAILLIGHT_PAGE (0xFF), TAILLIGHT_FEATURE (0x02) |
| `app/firefly.cpp` | User-mode control app (flicker.exe) |
| `shared/luminous.h` | User-mode WMI wrapper class (CLuminous) |

---

## 7. 與 M487 HID Filter Driver 的相關性

### 7.1 可直接借鑒的模式

| 需求 | Firefly 解決方案 |
|------|-----------------|
| 註冊為 upper filter | `WdfFdoInitSetFilter()` + INF UpperFilters |
| 向下層 HID 發送 IOCTL | `WdfIoTargetOpen(PDO name)` + `WdfIoTargetSendIoctlSynchronously()` |
| 取得 HID 描述符 | `IOCTL_HID_GET_COLLECTION_INFORMATION` + `IOCTL_HID_GET_COLLECTION_DESCRIPTOR` |
| 發送 feature report | `IOCTL_HID_SET_FEATURE` + `HidP_SetUsages()` |
| 讀取 feature report | `IOCTL_HID_GET_FEATURE` + `HidP_GetUsages()` |
| User-mode 控制 | WMI class registration + COM API |

### 7.2 需要修改的部分

1. **目標裝置**：從 Microsoft Optical Mouse 改為 M487 USB 裝置
2. **HID Usage Page/ID**：從 0xFF/0x02 改為 M487 的實際值
3. **INF 硬體 ID**：從 `HID\Vid_045E...` 改為 M487 的 VID/PID
4. **WMI GUID**：需要定義新的 GUID for M487
5. **通訊協議**：M487 的 command/response 格式可能不同

### 7.3 建議的 M487 實作架構

```
User-Mode App          Kernel Filter          M487 Device
     │                       │                       │
     │── WMI Set Feature ───▶│                       │
     │                       │── IOCTL_HID_SET...──▶│
     │                       │◀─── response ─────────│
     │◀─── WMI 確認 ─────────│                       │
```

---

## 8. 技術參考

- **WDK 文件：** [Creating Framework-based HID Minidrivers](https://docs.microsoft.com/previous-versions//ff540774(v=vs.85))
- **HID Class Driver:** `hidclass.sys` (Windows HID Class Driver)
- **KMDF Filter Driver:** `WdfFdoInitSetFilter()` (KMDF documentation)
- **HID API:** `HidP_GetCaps()`, `HidP_SetUsages()`, `HidP_GetUsages()` (HIDPI)
- **IOCTL:** `IOCTL_HID_GET_COLLECTION_INFORMATION`, `IOCTL_HID_SET_FEATURE`, etc.

---

## 9. 研究結論

Firefly 是一個**高度整合**的 KMDF HID upper filter driver 範例：

1. **KMDF 簡化了 filter driver 開發** — `WdfFdoInitSetFilter()` 處理了大部分 pass-through 邏輯
2. **WMI 是合適的 user/kernel 通訊介面** — 適用於需要非同步控制的場景
3. **WDF I/O Target 是關鍵 API** — 用於向 HID 設備主動發送 IOCTL
4. **HIDP API 提供了標準化的 Report 操作方式** — `HidP_SetUsages`/`HidP_GetUsages` 是操作 feature report 的正確方式
5. **這個範例對 M487 HID Filter Driver 開發有直接參考價值** — 特別是 IOCTL 發送流程和 WMI 介面設計
