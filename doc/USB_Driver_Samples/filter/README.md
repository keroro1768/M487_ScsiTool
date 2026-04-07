# USB Filter Driver 分析總覽

## 📋 概述

本目錄包含 USB 和一般 Filter Driver 的分析文件。Filter Driver 是附加至現有裝置堆疊以攔截和修改請求的驅動程式。

## 📁 目錄內容

| 檔案 | 描述 | 重要性 |
|------|------|--------|
| `README.md` | 本檔案 - Filter Driver 分析總覽 | - |
| `Filter_Driver_Pattern.md` | 常見 Filter Driver 模式與實作 | ⭐⭐⭐⭐ |

## 🔑 Filter Driver 類型

### 上層過濾器（Upper Filter）

附加在函式驅動程式之上，優先處理來自上層的請求。

```
┌─────────────────────┐
│    Highest Driver    │
├─────────────────────┤
│   Upper Filter      │ ← 這裡
├─────────────────────┤
│   Function Driver   │
├─────────────────────┤
│   Lower Filter     │
├─────────────────────┤
│    Bus Driver      │
└─────────────────────┘
```

### 下層過濾器（Lower Filter）

附加在匯流排驅動程式之上，優先處理來自下層的請求。

## 📂 相關範例

### 1. firefly（HID Upper Filter）

**位置：** `hid/firefly/`

KMDF HID 上層過濾驅動程式範例，展示：
- 如何附加至 HID 裝置堆疊
- 如何發送 HID IOCTL
- 如何使用 WMI 與使用者通訊

**詳細分析：** 見 `../hid/firefly_Deep_Dive.md`

### 2. toaster filter（通用 Filter）

**位置：** `general/toaster/toastDrv/kmdf/filter/generic/`

KMDF 通用 Filter Driver 範例，展示：
- 基本 Filter Driver 架構
- 請求轉發機制
- 完成例程處理

## 🔧 KMDF Filter Driver 關鍵 API

### 設定為 Filter Driver

```c
// 在 EvtDeviceAdd 中呼叫
WdfFdoInitSetFilter(DeviceInit);
```

### 請求轉發

```c
// 簡單轉發（Fire and Forget）
WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS);

// 帶完成例程的轉發
WdfRequestFormatRequestUsingCurrentType(Request);
WdfRequestSetCompletionRoutine(Request, CompletionRoutine, Context);
WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS);
```

### IO Target

```c
// 獲取下層 IO Target
WdfDeviceGetIoTarget(device)

// 開啟 PDO
WdfIoTargetCreate(...)
WdfIoTargetOpen(hidTarget, ...)
```

## 📝 INF 註冊方式

### Upper Filter

```inf
[MyDevice.NT.HW]
AddReg = MyDevice.HWAddReg.NT

[MyDevice.HWAddReg.NT]
HKR,,"UpperFilters",0x00010000,"MyFilterDriver"
```

### Lower Filter

```inf
[MyDevice.NT.HW]
AddReg = MyDevice.HWAddReg.NT

[MyDevice.HWAddReg.NT]
HKR,,"LowerFilters",0x00010000,"MyFilterDriver"
```

## 🔄 請求處理模式

### 1. 完全透明轉發

```c
VOID
FilterEvtIoRead(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t Length
    )
{
    WdfRequestForwardToIoQueue(Request, 
        WdfDeviceGetIoTarget(WdfIoQueueGetDevice(Queue)));
}
```

### 2. 修改後轉發

```c
VOID
FilterEvtIoDeviceControl(
    WDFQUEUE Queue,
    WDFREQUEST Request,
    size_t OutputBufferLength,
    size_t InputBufferLength,
    ULONG IoControlCode
    )
{
    if (IoControlCode == MY_CUSTOM_IOCTL) {
        // 修改請求
        ModifyRequest(Request);
    }
    
    // 轉發至下層
    FilterForwardRequest(Request, WdfDeviceGetIoTarget(device));
}
```

### 3. 帶完成的轉發

```c
VOID
FilterForwardRequestWithCompletionRoutine(
    WDFREQUEST Request,
    WDFIOTARGET Target
    )
{
    WdfRequestFormatRequestUsingCurrentType(Request);
    WdfRequestSetCompletionRoutine(Request, 
        FilterRequestCompletionRoutine, WDF_NO_CONTEXT);
    WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS);
}

VOID
FilterRequestCompletionRoutine(
    WDFREQUEST Request,
    WDFIOTARGET Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    WDFCONTEXT Context
    )
{
    // 在請求完成後處理
    if (NT_SUCCESS(CompletionParams->IoStatus.Status)) {
        // 後處理
    }
    WdfRequestComplete(Request, CompletionParams->IoStatus.Status);
}
```

## 📊 與 USB Filter 的關聯

### USB 堆疊 Filter 位置

```
┌─────────────────────────────────────────────────┐
│                  User Mode                        │
└────────────────────────┬────────────────────────┘
                         │
┌────────────────────────▼────────────────────────┐
│              Function Driver                      │
│         (例如 USBHUB.SYS 的上層)                 │
└────────────────────────┬────────────────────────┘
                         │
┌────────────────────────▼────────────────────────┐
│            Upper Filter (如 firefly)             │
│   - 攔截 HID 報告                               │
│   - 修改 Feature/Input/Output 報告              │
│   - 添加 WMI 介面                               │
└────────────────────────┬────────────────────────┘
                         │
┌────────────────────────▼────────────────────────┐
│            Function Driver                        │
│         (HID Class / USB Driver)                 │
└────────────────────────┬────────────────────────┘
                         │
┌────────────────────────▼────────────────────────┐
│            Lower Filter (optional)               │
│   - USB 層面的過濾                               │
└────────────────────────┬────────────────────────┘
                         │
┌────────────────────────▼────────────────────────┐
│              USB Hub Driver                       │
└─────────────────────────────────────────────────┘
```

### USB HID Filter 特殊考量

1. **IOCTL 處理** - HID 使用 `IRP_MJ_INTERNAL_DEVICE_CONTROL`
2. **報告緩衝區** - 可修改 Input/Output/Feature 報告
3. **PDO 開啟** - 需要以名稱開啟 PDO 才能發送 IOCTL

---

**最後更新：** 2026-04-07  
**版本：** 1.0.0  
**維護者：** KeroroTeam - Tamama
