# M487 HID Upper Filter Driver - T027d Complete

> **狀態：** 🟢 IOCTL 攔截實作完成（T027d）
> **Framework：** KMDF (Windows Driver Model)
> **模式：** HID Upper Filter Driver
> **目標：** Nuvoton M487 USB Device

---

## 架構

```
┌─────────────────────────────────────────┐
│  User-Mode Application                  │
│  (IOCTL / WMI API)                     │
└────────────┬────────────────────────────┘
             │
┌────────────▼────────────────────────────┐
│  M487Filter.sys (KMDF Upper Filter)     │
│  - WdfFdoInitSetFilter()                │
│  - Automatic IRP pass-through           │
│  - ★ IOCTL interception (SET/GET/       │
│    READ/WRITE_FEATURE)                  │
│  - ★ Ring buffer capture                │
│  - WMI: PassThroughEnabled toggle       │
│  - IOCTL: Custom commands               │
└────────────┬────────────────────────────┘
             │ IOCTL_HID_SET_FEATURE / IOCTL_HID_GET_*
             │ (Forwarding with capture)
┌────────────▼────────────────────────────┐
│  hidclass.sys (HID Class Driver)        │
└────────────┬────────────────────────────┘
             │ USB HRs
┌────────────▼────────────────────────────┐
│  usbd.sys + USB Hardware (M487)         │
└─────────────────────────────────────────┘
```

## 目錄結構

```
tool/M487Filter/
├── Task.md                    # T027g+T027h 任務追蹤
├── README.md
├── driver/
│   ├── M487Filter.sys         # Driver binary (WDK build 產出)
│   ├── M487Filter.cat         # Signed catalog (inf2cat+signtool 產出)
│   ├── M487Filter.inf         # Installation INF
│   ├── M487Filter.mof         # WMI MOF definition
│   ├── M487Filter.rc          # Version resource
│   ├── M487Filter.h           # Main header (includes all modules)
│   ├── M487Filter.c           # DriverEntry
│   ├── M487FilterDevice.h/c   # DEVICE_CONTEXT + EvtDriverDeviceAdd
│   ├── M487FilterIoctl.h/c    # Custom IOCTL + interception entry
│   ├── M487FilterIntercept.h/c# IOCTL interception (EvtIoDeviceControl)
│   ├── M487FilterRingBuffer.h/c # Ring buffer implementation
│   ├── M487FilterWmi.h/c      # WMI providers
│   ├── M487FilterEtw.h/c      # ETW tracing
│   ├── M487FilterMof.h        # WMI GUID definitions
│   ├── SOURCES                # WDK build file
│   ├── CMakeLists.txt         # CMake build file
│   ├── makefile               # WDK makefile
│   ├── install.bat            # 安裝腳本（T027h 新增）
│   ├── uninstall.bat          # 解除安裝腳本（T027h 新增）
│   └── SIGNING.md             # 驅動程式簽章指南（T027h 新增）
└── app/
    ├── hidlog.c               # CLI 工具（T027g）
    ├── M487FilterWmiApp.c      # WMI Query/Set app（T027f）
    ├── CMakeLists.txt          # App CMake build
    └── build.cmd              # 建置腳本（T027g）
```

## 已實作功能

### IOCTL 截獲（T027d）
| IOCTL | 方向 | 說明 |
|-------|------|------|
| `IOCTL_HID_SET_FEATURE` | Host → Device | Feature Report 發送（截獲 + 轉送）|
| `IOCTL_HID_GET_FEATURE` | Host ← Device | Feature Report 讀取（截獲回傳值）|
| `IOCTL_HID_WRITE_REPORT` | Host → Device | Output Report 發送（截獲 + 轉送）|
| `IOCTL_HID_READ_REPORT` | Host ← Device | Input Report 讀取（截獲回傳值）|

### Ring Buffer 捕獲
- **256 slots**，每 slot 最大 64 bytes
- **Lock-free** 設計（InterlockedXXX 操作）
- **Magic signature** 驗證初始化
- User-mode 可透過 `IOCTL_M487FILTER_GET_RING_ADDR` 取得實體位址後 `MmMapIoSpace` 映射

### User-Mode IOCTL API
| IOCTL | 說明 |
|-------|------|
| `IOCTL_M487FILTER_GET_INFO` | +含捕獲統計（TotalCaptured/DroppedCount/SlotsUsed）|
| `IOCTL_M487FILTER_SET_MODE` | 設定濾波模式 |
| `IOCTL_M487FILTER_SEND_FEATURE` | 發送 Feature Report |
| `IOCTL_M487FILTER_GET_RING_STATS` | 取得 Ring Buffer 統計 |
| `IOCTL_M487FILTER_GET_RING_ADDR` | 取得 Ring Buffer 實體位址與大小 |
| `IOCTL_M487FILTER_SET_INTERCEPT_CONFIG` | 設定截獲開關（SET/GET/READ/WRITE）|
| `IOCTL_M487FILTER_GET_INTERCEPT_CONFIG` | 取得目前截獲配置 |

## Firefly IOCTL 流程（參考）

```c
// IOCTL_HID_SET_FEATURE 流程
WdfIoTargetOpen(PDO name)
  → IOCTL_HID_GET_COLLECTION_INFORMATION
  → IOCTL_HID_GET_COLLECTION_DESCRIPTOR
  → HidP_GetCaps()
  → IOCTL_HID_SET_FEATURE  ← M487Filter 截獲此處
```

## 支援的 VID/PID

| VID | PID | 說明 |
|-----|-----|------|
| 0x0416 | 0x501E | M487 USB Storage Device (HID) |
| 0x0416 | 0xFF20 | M487 Vendor Loopback (測試用) |
| 0x0416 | 0x5020 | M487 Composite Device (測試用) |

> ⚠️ **注意：** 以上 PID 需根據實際 M487 firmware 的 HID 介面 VID/PID 進行調整。

## 編譯

### WDK 命令列編譯

```powershell
cd D:\AiWorkSpace\M487_ScsiTool\tool\M487Filter\driver
build -g -km 1> build.log 2>&1
```

### 需要的 WDK 標頭

- `ntddk.h` - Kernel mode APIs
- `wdf.h` - Windows Driver Frameworks
- `hidpddi.h` - HID Parser APIs
- `hidclass.h` - HID Class Driver IOCTLs

## 安裝

### 快速安裝（自動偵測）

```powershell
cd driver
install.bat
```

### 測試簽章模式（開發用）

```powershell
# 第一次：啟用測試簽章（需重開機）
bcdedit /set testsigning on
reboot

# 重新開機後，安裝驅動
cd driver
install.bat test-sign
```

### 正式環境（需 EV Code Signing 憑證）

```powershell
install.bat ev-sign "C:\certs\ev-cert.pfx" "password"
```

### 手動安裝

1. 複製 `M487Filter.sys`、`M487Filter.inf`、`M487Filter.cat` 到同一目錄
2. 右鍵點擊 INF > **安裝**
3. 或使用 `devcon`：

```powershell
devcon install M487Filter.inf "HID\Vid_0416&Pid_501E"
```

### 解除安裝

```powershell
cd driver
uninstall.bat
```

## 驅動程式簽章

詳細簽章流程請參考：**[SIGNING.md](driver/SIGNING.md)**

| 方法 | 適用場景 | 成本 |
|------|----------|------|
| 測試簽章 | 開發/除錯 | 免費 |
| EV Code Signing | 正式環境 | $300-500/年 |
| Microsoft HW Dev Center | 長期正式/HLK | 費用另計 |

## 待完成

- [ ] 確認 M487 HID 介面的實際 VID/PID
- [ ] WDK 編譯驗證（M487Filter.sys 產出）
- [ ] inf2cat 執行確認（M487Filter.cat 產生）
- [ ] 測試簽章驅動程式實際載入驗證
- [ ] 實際設備上的 IOCTL 截獲測試驗證
- [ ] User-mode 控制應用程式讀取 Ring Buffer
- [ ] hidlog.exe -f 模式持續監控驗證
- [ ] WMI Query/Set 實際運作驗證
- [ ] 多執行緒並發安全性驗證
- [ ] 低速設備條件下的 DroppedCount 行為
- [ ] Report Descriptor 解析（用於 Usage 等級過濾）
- [ ] EV Code Signing 正式憑證申請與安裝流程
- [ ] HLK 驗證

## 參考

- Microsoft Windows-driver-samples: `hid/firefly/`
- WDK Documentation: [Filter Drivers](https://docs.microsoft.com/windows-hardware/drivers/kernel/filter-drivers)
- KMDF: [WdfFdoInitSetFilter function](https://docs.microsoft.com/windows-hardware/drivers/ddi/wdffdo/nf-wdffdo-wdffdoinitsetfilter)
- KMDF: [WdfDeviceConfigureRequestDispatching](https://docs.microsoft.com/windows-hardware/drivers/ddi/wdffdo/nf-wdffdo-wdfdeviceconfigurerequestdispatching)
