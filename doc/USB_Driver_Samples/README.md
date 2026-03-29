# USB Driver Samples 研究報告

> 分析微軟 Windows-driver-samples 中 USB/HID 驅動程式範例  
> 適用於 M487 USB 開發專案參考

---

## 📁 範例目錄結構總覽

```
Windows-driver-samples/usb/
├── usbsamp/          # 基本 Bulk/Interrupt/Isochronous USB 範例（KMDF）
├── usbview/          # USB 裝置檢視工具（User Mode + WDM）
├── kmdf_fx2/         # OSR USB-FX2 開發板（KMDF）
├── umdf2_fx2/        # OSR USB-FX2 開發板（UMDF）
├── kmdf_enumswitches/ # 列舉交換器範例
├── wdf_osrfx2_lab/   # Step-by-step 實驗室教材
├── UcmCxUcsi/        # USB Connector Manager (UCM) + UCSI
├── UcmTcpciCxClientSample/  # USB Type-C
└── ufxclientsample/  # USB Function Client Sample

Windows-driver-samples/hid/
├── hclient/          # HID Client 範例（User Mode）
├── firefly/          # 螢火蟲感測器
├── hidusbfx2/        # HID over USB FX2
└── vhidmini2/        # 虛擬 HID Mini
```

> ⚠️ `hid/kbd/` 目錄不存在，鍵盤範例已不存在於本版本 samples 中。

---

## 1. usbsamp — 基本 USB 裝置驅動（KMDF）

**路徑**: `usb/usbsamp/sys/`

### 架構摘要

這是 Intel 82930 USB 測試板的範例驅動，支援 **Bulk / Interrupt / Isochronous** 三種傳輸類型，是最完整的基礎 USB 驅動範例。

### 核心模組

| 檔案 | 職責 |
|------|------|
| `driver.c` | DriverEntry + WDF_DRIVER_CONFIG 初始化 |
| `device.c` | PnP Manager 回調（EvtDeviceAdd / EvtDevicePrepareHardware）、描述符讀取、介面選擇 |
| `queue.c` | IRP 派發（EvtIoRead / Write / DeviceControl、EvtDeviceFileCreate） |
| `bulkrwr.c` | Bulk Read/Write 傳輸實作 |
| `isorwr.c` | Isochronous Read/Write 傳輸實作 |
| `private.h` | DEVICE_CONTEXT / PIPE_CONTEXT / REQUEST_CONTEXT / FILE_CONTEXT 定義 |

### 關鍵程式碼模式

#### 1.1 USB 描述符設定（Configuration/Interface/Endpoint）

**`ReadAndSelectDescriptors` → `ConfigureDevice` → `SelectInterfaces`** 流程：

```c
// 1. 建立 WDFUSBDEVICE 句柄
WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config, USBD_CLIENT_CONTRACT_VERSION_602);
status = WdfUsbTargetDeviceCreateWithParameters(Device, &config, ...,
    &pDeviceContext->WdfUsbTargetDevice);

// 2. 讀取 Configuration Descriptor（全貌）
status = WdfUsbTargetDeviceRetrieveConfigDescriptor(pDeviceContext->WdfUsbTargetDevice,
    configurationDescriptor, &size);

// 3. 驗證描述符合法性
status = UsbSamp_ValidateConfigurationDescriptor(configurationDescriptor, size, &Offset);

// 4. 選擇 Configuration + Interface
WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->WdfUsbTargetDevice,
    &pipeAttributes, &configParams);

// 5. 列舉所有 Pipe（Endpoint）
for (i = 0; i < numberConfiguredPipes; i++) {
    pipe = WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface, i, NULL);
    // 依速度初始化 Pipe Context
}
```

#### 1.2 URB 處理流程（KMDF 封裝）

KMDF 將 URB 操作封裝為高階 API，無需直接操作 URB：

```c
// Bulk Read（同步）
status = WdfUsbTargetPipeFormatRequestForRead(pipe, Request, reqMemory, NULL);
WdfRequestSetCompletionRoutine(Request, EvtRequestReadCompletionRoutine, pipe);
WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS);

// Bulk Write（同步）
status = WdfUsbTargetPipeFormatRequestForWrite(pipe, Request, reqMemory, NULL);
WdfRequestSetCompletionRoutine(Request, EvtRequestWriteCompletionRoutine, pipe);
WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS);

// Control Transfer（同步 Vendor Command）
WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
    BmRequestHostToDevice, BmRequestToDevice, RequestCode, Value, Index);
status = WdfUsbTargetDeviceSendControlTransferSynchronously(
    pDeviceContext->WdfUsbTargetDevice, NULL, &sendOptions,
    &controlSetupPacket, &memDesc, &bytesTransferred);

// Pipe Abort
status = WdfUsbTargetPipeAbortSynchronously(pipe, WDF_NO_HANDLE, NULL);

// Pipe Reset
status = WdfUsbTargetPipeResetSynchronously(Pipe, WDF_NO_HANDLE, NULL);
```

#### 1.3 Isochronous Transfer（等時傳輸）

```c
// 等時傳輸需要特殊的 Frame Number 管理
VOID PerformIsochTransfer(PDEVICE_CONTEXT DeviceContext, WDFREQUEST Request, ULONG TotalLength)
{
    // 依速度計算每 Frame/Microframe 的傳輸量
    // High Speed: TransferSizePerMicroframe = MaximumPacketSize
    // Full Speed: TransferSizePerFrame = MaximumPacketSize
    // Super Speed: TransferSizePerMicroframe = wBytesPerInterval (from companion desc)
    
    // WdfUsbTargetPipeFormatRequestForUrb() + USBD_AllocateUsbBuffer()
    // 手動設定 StartFrame（需排程計算避免重疊）
}
```

#### 1.4 PnP / Power Management

```c
// Power Policy 設定（Idle + Wake）
WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleUsbSelectiveSuspend);
idleSettings.IdleTimeout = 10000; // 10秒
WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);
WdfDeviceAssignSxWakeSettings(Device, &wakeSettings);

// USBD_HANDLE 用於低階描述符解析
USBD_CreateHandle(WdfDeviceWdmGetDeviceObject(device), ..., USBD_CLIENT_CONTRACT_VERSION_602, ...);
```

#### 1.5 Pipe Context（依速度差異）

```c
// High Speed Pipe Init（等時）
if (pipeInfo.MaximumPacketSize > 1024 * 3) return STATUS_INVALID_PARAMETER; // max 3x HS
if (pipeInfo.Interval == 0 || pipeInfo.Interval > 4) return STATUS_INVALID_PARAMETER; // 僅支援 1,2,3,4
pipeContext->TransferSizePerMicroframe = pipeInfo.MaximumPacketSize;

// Super Speed Pipe Init（等時 + Companion Descriptor）
pEndpointCompanionDescriptor = GetEndpointDescriptorForEndpointAddress(...);
wBytesPerInterval = pEndpointCompanionDescriptor->wBytesPerInterval;
// TransferSizePerMicroframe = wBytesPerInterval
```

### 適用情境

- ✅ **最佳參考起點**：涵蓋所有三種 USB 傳輸類型
- ✅ Bulk 資料傳輸（最常用）
- ✅ Interrupt 端點（如按鈕/開關通知）
- ✅ Isochronous（音訊/視訊）等時傳輸
- ✅ UVC（USB Video Class）等高速等時應用
- ✅ USB 描述符解析與驗證
- ✅ 多種速度支援（Full/High/SuperSpeed）

---

## 2. kmdf_fx2 — OSR USB-FX2 開發板（KMDF）

**路徑**: `usb/kmdf_fx2/driver/`

### 架構摘要

專為 OSR USB-FX2 Learning Kit 設計，**Bulk IN/OUT + Interrupt IN** 三端點，支援讀取開關狀態、七段顯示器、LED Bar Graph。是最乾淨的教學級 USB 驅動範例。

### 核心模組

| 檔案 | 職責 |
|------|------|
| `driver.c` | DriverEntry、WPP Tracing 初始化 |
| `Device.c` | EvtDeviceAdd、EvtDevicePrepareHardware、SelectInterfaces、Power Policy |
| `ioctl.c` | IOCTL 處理、Control Transfer 實作（Vendor Commands）、Pipe Reset |
| `interrupt.c` | Continuous Reader 設定（等時中斷讀取） |
| `bulkrwr.c` | Bulk Read/Write |
| `osrusbfx2.h` | DEVICE_CONTEXT（含 UsbDevice/Interface/Pipe 句柄）、vendor command 定義 |
| `osrusbfx2.man` | ETW Event Manifest |

### 關鍵程式碼模式

#### 2.1 USB 描述符設定

```c
// WDF_USB_DEVICE_CREATE_CONFIG_INIT + WdfUsbTargetDeviceCreateWithParameters
// 等同於 usbsamp，但更簡潔

// 單一介面選擇
WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);
WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice, WDF_NO_OBJECT_ATTRIBUTES, &configParams);

// Pipe 枚舉 + 分類儲存
for (index = 0; index < numberConfiguredPipes; index++) {
    pipe = WdfUsbInterfaceGetConfiguredPipe(pDeviceContext->UsbInterface, index, &pipeInfo);
    if (WdfUsbPipeTypeInterrupt == pipeInfo.PipeType) pDeviceContext->InterruptPipe = pipe;
    if (WdfUsbPipeTypeBulk == pipeInfo.PipeType && WdfUsbTargetPipeIsInEndpoint(pipe)) 
        pDeviceContext->BulkReadPipe = pipe;
    if (WdfUsbPipeTypeBulk == pipeInfo.PipeType && WdfUsbTargetPipeIsOutEndpoint(pipe)) 
        pDeviceContext->BulkWritePipe = pipe;
}
```

#### 2.2 Continuous Reader（Interrupt Pipe）

這是 kmdf_fx2 最具價值的特色——使用連續讀取器處理中斷端點：

```c
// 設定連續讀取器（等時輪詢中斷端點）
NTSTATUS OsrFxConfigContReaderForInterruptEndPoint(PDEVICE_CONTEXT DeviceContext)
{
    WDF_USB_CONTINUOUS_READER_CONFIG contReaderConfig;
    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&contReaderConfig,
        OsrFxEvtUsbInterruptPipeReadComplete,  // 完成回調
        DeviceContext, sizeof(UCHAR));
    contReaderConfig.EvtUsbTargetPipeReadersFailed = OsrFxEvtUsbInterruptReadersFailed;
    
    return WdfUsbTargetPipeConfigContinuousReader(DeviceContext->InterruptPipe, &contReaderConfig);
}

// D0Entry 啟動 / D0Exit 停止
WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(pDeviceContext->InterruptPipe));
WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(pDeviceContext->InterruptPipe), WdfIoTargetCancelSentIo);

// 完成回調中讀取開關狀態
VOID OsrFxEvtUsbInterruptPipeReadComplete(WDFUSBPIPE Pipe, WDFMEMORY Buffer, 
    size_t NumBytesTransferred, WDFCONTEXT Context)
{
    PUCHAR switchState = WdfMemoryGetBuffer(Buffer, NULL);
    pDeviceContext->CurrentSwitchState = *switchState;
    OsrUsbIoctlGetInterruptMessage(device, STATUS_SUCCESS); // 喚醒等待中的 IOCTL
}
```

#### 2.3 Vendor Control Transfer

```c
// 發送 Vendor Command（讀取七段顯示器狀態）
WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
    BmRequestDeviceToHost,     // 方向：Device → Host
    BmRequestToDevice,          // 目標：裝置
    USBFX2LK_READ_7SEGMENT_DISPLAY, // 0xD4
    0, 0);

status = WdfUsbTargetDeviceSendControlTransferSynchronously(
    DevContext->UsbDevice, NULL, &sendOptions, &controlSetupPacket, &memDesc, &bytesTransferred);

// 發送 Vendor Command（設定 LED Bar Graph）
WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
    BmRequestHostToDevice, BmRequestToDevice,
    USBFX2LK_SET_BARGRAPH_DISPLAY, 0, 0);
```

#### 2.4 IOCTL 派發模式

```c
// IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE — 將請求排入中斷訊息佇列
case IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE:
    status = WdfRequestForwardToIoQueue(Request, pDevContext->InterruptMsgQueue);
    if (NT_SUCCESS(status)) requestPending = TRUE;
    break;

// 其他 IOCTL 完成後直接回應
WdfRequestCompleteWithInformation(Request, status, bytesReturned);
```

#### 2.5 PnP / Power Management

```c
// 驚嘆移除設定（防止使用者彈出視窗）
WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
pnpCaps.SurpriseRemovalOK = WdfTrue;
WdfDeviceSetPnpCapabilities(device, &pnpCaps);

// Idle Timeout (Selective Suspend)
idleSettings.IdleTimeout = 10000; // 10 秒
WdfDeviceAssignS0IdleSettings(Device, &idleSettings);

// D0Entry / D0Exit 配合連續讀取器
```

### 適用情境

- ✅ **最佳教學範例**：程式碼精簡、架構清晰
- ✅ Interrupt 端點 + Continuous Reader 模式
- ✅ Vendor Command 實作
- ✅ 開發板（如 CY7C64215/FTDI FT2232 之類的 FX2 兼容板）
- ✅ 學習 WDF USB 完整生命週期

---

## 3. umdf2_fx2 — OSR USB-FX2（UMDF 版本）

**路徑**: `usb/umdf2_fx2/driver/`

### 架構摘要

幾乎與 kmdf_fx2 相同，但使用 **User Mode Driver Framework (UMDF 2.x)**。適合參考 UMDF 與 KMDF 的差異。

### 與 KMDF 版本差異

| 項目 | KMDF | UMDF 2.x |
|------|------|----------|
| IRQL | 可在 DISPATCH_LEVEL | 僅 PASSIVE_LEVEL |
| 記憶體配置 | 可用 NonPagedPoolNx | 使用 WdfMemoryCreate |
| 同步 | 可用自旋鎖 | 僅能使用協力廠商同步物件 |
| WDFUSBDEVICE | Kernel WDFUSBDEVICE | WinRT COM-style I/O target |
| COM 要求 | 無 | IClassProvider 等 COM 接口 |

---

## 4. usbview — USB 檢視工具（User Mode WDM）

**路徑**: `usb/usbview/`

### 架構摘要

純 **User Mode** 應用程式，使用 WDM API 直接遍歷所有 USB 匯流排、解析描述符鏈，顯示樹狀拓撲。含 USB 攝影機（UVC）和音訊類描述符解析。

### 關鍵程式碼模式

#### 4.1 PnP 裝置列舉

```c
// 使用 SetupAPI 列舉所有 HID/USB 裝置
SetupDiGetClassDevs(&hidGuid, NULL, NULL, 
    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

// 使用 CM (Configuration Manager) API 遍歷 USB 樹
CM_Get_Child(&dnDevInst, dnParent, 0);
CM_Get_Sibling(&dnDevInst, dnDevInst, 0);
CM_Get_Device_ID(dnDevInst, ...);

// 透過 WDM IRP 查詢描述符
DeviceIoControl(hHubDevice, IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION, ...);
```

#### 4.2 描述符類型定義

`usbdesc.h` 定義了多個標準中未收錄的描述符型態：

```c
// USB_HID_DESCRIPTOR（不在標準 usb100.h 中）
typedef struct _USB_HID_DESCRIPTOR {
    UCHAR bLength; UCHAR bDescriptorType; USHORT bcdHID;
    UCHAR bCountryCode; UCHAR bNumDescriptors;
    struct { UCHAR bDescriptorType; USHORT wDescriptorLength; } OptionalDescriptors[1];
} USB_HID_DESCRIPTOR;

// USB_IAD_DESCRIPTOR（用於 Composite Device）
typedef struct _USB_IAD_DESCRIPTOR {
    UCHAR bLength; UCHAR bDescriptorType; UCHAR bFirstInterface;
    UCHAR bInterfaceCount; UCHAR bFunctionClass; UCHAR bFunctionSubClass;
    UCHAR bFunctionProtocol; UCHAR iFunction;
} USB_IAD_DESCRIPTOR;

// Audio Class 描述符（Audio Control / Audio Streaming / MIDI）
// USB_AUDIO_AC_INTERFACE_HEADER_DESCRIPTOR
// USB_AUDIO_INPUT_TERMINAL_DESCRIPTOR / OUTPUT_TERMINAL_DESCRIPTOR
// USB_AUDIO_FEATURE_UNIT_DESCRIPTOR
// USB_AUDIO_AS_GENERAL / TYPE_I_or_III_FORMAT_DESCRIPTOR
```

### 適用情境

- ✅ **理解 USB 描述符結構**的最佳工具
- ✅ 分析陌生 USB 裝置的描述符
- ✅ 學習 IAD（Interface Association Descriptor）
- ✅ Audio Class / Video Class 描述符解析
- ✅ USB 拓撲結構視覺化

---

## 5. hclient — HID Client 範例（User Mode）

**路徑**: `hid/hclient/`

### 架構摘要

User Mode 應用程式，透過 **HID API**（hid.dll / hidclass.sys）與 HID 裝置通訊。示範如何列舉、讀取 Feature/Input/Output Report，解析 HID Report Descriptor。

### 核心模組

| 檔案 | 職責 |
|------|------|
| `pnp.c` | 裝置列舉（SetupDi + CreateFile + HidD_GetPreparsedData） |
| `report.c` | Report 讀寫 + 解包（HidP_GetXxx） |
| `hclient.c` | 主對話方塊 + UI 事件處理 |
| `buffers.c` / `ecdisp.c` | 緩衝區管理 + 特殊顯示 |

### 關鍵程式碼模式

#### 5.1 HID 裝置列舉

```c
// 取得 HID GUID
HidD_GetHidGuid(&hidGuid);

// SetupDi 列舉所有 HID 介面
SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

// 開啟裝置
HidDevice->HidDevice = CreateFile(DevicePath, GENERIC_READ | GENERIC_WRITE,
    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

// 取得描述符
HidD_GetPreparsedData(HidDevice->HidDevice, &HidDevice->Ppd);
HidD_GetAttributes(HidDevice->HidDevice, &HidDevice->Attributes);
HidP_GetCaps(HidDevice->Ppd, &HidDevice->Caps);
```

#### 5.2 HID Report Descriptor 解析

```c
// 查詢 Button Caps 和 Value Caps
numCaps = HidDevice->Caps.NumberInputButtonCaps;
buttonCaps = calloc(numCaps, sizeof(HIDP_BUTTON_CAPS));
HidP_GetButtonCaps(HidP_Input, buttonCaps, &numCaps, HidDevice->Ppd);

numCaps = HidDevice->Caps.NumberInputValueCaps;
valueCaps = calloc(numCaps, sizeof(HIDP_VALUE_CAPS));
HidP_GetValueCaps(HidP_Input, valueCaps, &numCaps, HidDevice->Ppd);

// 展開 Range（如 UsageMin ~ UsageMax）為獨立 Data 項目
if (valueCaps->IsRange) {
    numValues += valueCaps->Range.UsageMax - valueCaps->Range.UsageMin + 1;
}
```

#### 5.3 HID Report 讀取

```c
// 直接讀取（同步）
ReadFile(HidDevice->HidDevice, HidDevice->InputReportBuffer,
    HidDevice->Caps.InputReportByteLength, &bytesRead, NULL);

// 解包 Report 為有意義的資料
UnpackReport(HidDevice->InputReportBuffer, HidDevice->Caps.InputReportByteLength,
    HidP_Input, HidDevice->InputData, HidDevice->InputDataLength, HidDevice->Ppd);

// 讀取 Feature Report
HidD_GetFeature(HidDevice->HidDevice, HidDevice->FeatureReportBuffer,
    HidDevice->Caps.FeatureReportByteLength);

// 寫入 Output Report
WriteFile(HidDevice->HidDevice, HidDevice->OutputReportBuffer,
    HidDevice->Caps.OutputReportByteLength, &bytesWritten, NULL);
```

### 適用情境

- ✅ **HID 類裝置（M487 的 USB HID 介面）** 最佳參考
- ✅ 學習 HID Report Descriptor 結構
- ✅ 按鈕/開關等 Binary Input 處理
- ✅ Feature Report 用於設定
- ✅ User Mode 應用程式 + HID class driver 協作

---

## 6. 關鍵程式碼模式總整理

### 6.1 USB 描述符設定流程（KMDF）

```
DriverEntry
  └─ WdfDriverCreate()
  └─ EvtDeviceAdd (PnP Manager 回調)
       ├─ WdfDeviceCreate()
       ├─ WdfDeviceCreateDeviceInterface()       // 註冊 GUID 介面
       ├─ WdfIoQueueCreate()                     // 建立 I/O 佇列
       └─ EvtDevicePrepareHardware
            ├─ WdfUsbTargetDeviceCreateWithParameters()
            │    └─ WDF_USB_DEVICE_CREATE_CONFIG_INIT(version)
            ├─ WdfUsbTargetDeviceRetrieveConfigDescriptor()
            ├─ UsbSamp_ValidateConfigurationDescriptor()
            │    └─ USBD_ValidateConfigurationDescriptor()
            ├─ SelectInterfaces()
            │    ├─ WdfUsbTargetDeviceSelectConfig()
            │    ├─ WdfUsbInterfaceSelectSetting()   // 遍歷所有 Alternate Settings
            │    └─ WdfUsbInterfaceGetConfiguredPipe() // 取得 Pipe 句柄
            └─ OsrFxConfigContReaderForInterruptEndPoint()  // Interrupt Continuous Reader
```

### 6.2 URB 處理對照表（KMDF 封裝 vs WDM 直接呼叫）

| 操作 | KMDF API | 對應 URB_FUNCTION |
|------|----------|-------------------|
| 建立 USB 裝置 | `WdfUsbTargetDeviceCreateWithParameters()` | `URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE` |
| 選擇 Configuration | `WdfUsbTargetDeviceSelectConfig()` | `URB_FUNCTION_SET_CONFIGURATION` |
| 選擇 Interface | `WdfUsbInterfaceSelectSetting()` | `URB_FUNCTION_SELECT_INTERFACE` |
| Control Transfer | `WdfUsbTargetDeviceSendControlTransferSynchronously()` | `URB_FUNCTION_CONTROL_TRANSFER` |
| Bulk Read | `WdfUsbTargetPipeFormatRequestForRead()` + `WdfRequestSend()` | `URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER` |
| Bulk Write | `WdfUsbTargetPipeFormatRequestForWrite()` + `WdfRequestSend()` | `URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER` |
| Isoch Transfer | `WdfUsbTargetPipeFormatRequestForUrb()` | `URB_FUNCTION_ISOCH_TRANSFER` |
| Pipe Reset | `WdfUsbTargetPipeResetSynchronously()` | `URB_FUNCTION_RESET_PIPE` |
| Pipe Abort | `WdfUsbTargetPipeAbortSynchronously()` | `URB_FUNCTION_ABORT_PIPE` |
| 描述符驗證 | `USBD_ValidateConfigurationDescriptor()` | N/A（USBD helper） |

### 6.3 WDF USB 物件層次

```
WDFDRIVER
  └─ WDFDEVICE
       ├─ WDFUSBDEVICE        (pDeviceContext->UsbDevice)
       │    ├─ WDFUSBINTERFACE   (pDeviceContext->UsbInterface)
       │    │    ├─ WDFUSBPIPE   (InterruptPipe)
       │    │    ├─ WDFUSBPIPE   (BulkReadPipe)
       │    │    └─ WDFUSBPIPE   (BulkWritePipe)
       │    └─ 其他 Configuration 資訊
       └─ WDFQUEUE (default / read / write / interrupt-msg)
```

### 6.4 IRP 堆疊管理（KMDF）

KMDF 大幅簡化了 IRP 堆疊管理，驅動程式通常只需要：

```c
// 1. 派發到佇列
EvtIoRead / EvtIoWrite / EvtIoDeviceControl
  └─ WdfRequestForwardToIoQueue() / WdfRequestCompleteWithInformation()

// 2. 格式化 USB 請求
WdfUsbTargetPipeFormatRequestForRead/Write()

// 3. 設定完成回調
WdfRequestSetCompletionRoutine(Request, CompletionRoutine, Context);

// 4. 傳送
WdfRequestSend(Request, WdfUsbTargetPipeGetIoTarget(pipe), WDF_NO_SEND_OPTIONS);

// 5. 完成
EvtRequestReadCompletionRoutine()
  └─ WdfRequestCompleteWithInformation(Request, status, bytesTransferred);
```

### 6.5 PnP / Power 狀態機

```
EvtDeviceAdd ──────────────────────────────→ 裝置建立
        │
EvtDevicePrepareHardware ──────────────────→ 硬體初始化（描述符讀取）
        │
EvtDeviceD0Entry ←───────── [Wake from Sx] ─┐
        │                                     │
EvtDeviceSelfManagedIoFlush ←── [Remove] ────┤
        │                                     │
EvtDeviceD0Exit ──────────────→ [Sx / Remove] ┘
        │
EvtDeviceReleaseHardware ───────────────────→ 硬體釋放
```

### 6.6 IOCTL 設計模式

```c
// KMDF IOCTL 處理
EvtIoDeviceControl(Queue, Request, OutputBufferLength, InputBufferLength, IoControlCode)
{
    switch(IoControlCode) {
        case IOCTL_XXX_GET_CONFIG_DESCRIPTOR:
            status = WdfUsbTargetDeviceRetrieveConfigDescriptor(...);
            bytesReturned = requiredSize;
            break;
        case IOCTL_XXX_READ_SWITCHES:
            status = WdfRequestRetrieveOutputBuffer(Request, sizeof(SWITCH_STATE), &buf, NULL);
            status = GetSwitchState(pDevContext, buf);
            if (NT_SUCCESS(status)) bytesReturned = sizeof(SWITCH_STATE);
            break;
        case IOCTL_XXX_GET_INTERRUPT_MESSAGE:
            // 掛起直到中斷發生
            WdfRequestForwardToIoQueue(Request, InterruptMsgQueue);
            requestPending = TRUE;
            break;
    }
    if (!requestPending) {
        WdfRequestCompleteWithInformation(Request, status, bytesReturned);
    }
}
```

---

## 7. 與 M487 USB 開發的相關性

M487 基於 **Nuvoton M487KIDAE**（USB Host/Device），預期使用 **USB HID Class** 或 **USB Bulk** 介面。

### 建議參考組合

| M487 應用場景 | 優先參考範例 | 關鍵學習點 |
|-------------|------------|----------|
| USB HID Keyboard/Mouse | `hclient` + `hidusbfx2` | HID Report Descriptor、HIDP_* API |
| USB Bulk Data 傳輸 | `usbsamp` + `kmdf_fx2` | Bulk Read/Write、URI 處理 |
| Interrupt 端點通知 | `kmdf_fx2/interrupt.c` | Continuous Reader 模式 |
| Vendor-specific 指令 | `kmdf_fx2/ioctl.c` | Control Transfer + Vendor Command |
| PnP / Power | `usbsamp/device.c` | Selective Suspend、Idle Policy |
| 描述符解析 | `usbview` | Configuration/Interface/Endpoint Descriptor |

### M487 USB HID 實作重點

1. **描述符設定**：參考 `kmdf_fx2 SelectInterfaces()` — 使用 KMDF USB API
2. **Interrupt IN（鍵盤事件通知）**：使用 `OsrFxConfigContReaderForInterruptEndPoint()` 模式
3. **HID Report Descriptor**：參考 `hclient` 的 `FillDeviceInfo()` + `HidP_GetButtonCaps/ValueCaps`
4. **Vendor Command**：參考 `kmdf_fx2 ioctl.c` 的 `WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR`
5. **Idle/Suspend**：參考 `usbsamp SetPowerPolicy()` + `WdfDeviceAssignS0IdleSettings`

---

## 8. 其他值得關注的範例

### kmdf_enumswitches
USB 交換器列舉範例，學習 USB Hub 拓撲遍歷。

### wdf_osrfx2_lab
**step1 ~ step5** 漸進式學習教材，從零開始建構 USB 驅動，適合初學者。

### vhidmini2
虛擬 HID Miniport（KMDF + UMDF 兩版本），學習不依附實際硬體的 HID 驅動。

### firefly
含感測器讀取 + Android 應用的完整系統範例，含 Sauron 影像分析。

---

*研究時間：2026-03-28 | 來源：Microsoft Windows-driver-samples (GitHub)*
