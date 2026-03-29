# T027g - hidlog.exe CLI 工具

> **任務狀態：** ✅ 已完成
> **日期：** 2026-03-28
> **依賴任務：** T027f（WMI Interface 完成）

---

## 完成項目

### 1. hidlog.exe CLI 工具
- **`app/hidlog.c`** - 全新 CLI 工具原始碼：
  - 讀取 M487Filter ring buffer 中的 captured HID 封包
  - Hexdump 格式輸出（16 bytes per line, ASCII sidebar）
  - Filter by report type:
    - `set-feature` (IOCTL_HID_SET_FEATURE)
    - `get-feature` (IOCTL_HID_GET_FEATURE)
    - `write-report` (IOCTL_HID_WRITE_REPORT)
    - `read-report` (IOCTL_HID_READ_REPORT)
    - `all` (default)
  - 支援 ANSI 色彩輸出（Windows Terminal 支援）
  - 支援 `-f, --follow` 持續 tail 模式
  - 支援 `-n, --lines N` 顯示最近 N 筆
  - 支援 `-s, --stats` 只顯示統計資訊
  - 支援 `-q, --quiet` 安靜模式（純 hex 輸出）
  - 支援 `--no-color` 禁用色彩

- **`app/CMakeLists.txt`** - 更新：
  - 加入 hidlog.exe 編譯目標
  - 連結 advapi32.lib

- **`app/build.cmd`** - 新建建置腳本：
  - 支援 `build.cmd hidlog` 單獨編譯
  - 支援 `build.cmd wmiapp` 編譯 WMI app
  - 支援 `build.cmd clean` 清理
  - 自動偵測 Visual Studio 路徑

### 2. Driver 端 Ring Buffer User-Mode Mapping 支援
- **`driver/M487FilterIntercept.h`** - 更新：
  - `M487_INTERCEPT_STATE` 新增 `RingBufferSectionHandle` + `RingBufferUserVa`
  - 新增 `M487InterceptMapRingBufferToUser()` 函式宣告

- **`driver/M487FilterIntercept.c`** - 更新：
  - 新增 `ZwCreateSection`/`ZwMapViewOfSection`/`ZwUnmapViewOfSection` syscall 宣告
  - 實作 `M487InterceptMapRingBufferToUser()`:
    - 使用 `ZwCreateSection` 建立 memory section
    - 使用 `ZwMapViewOfSection` 映射到 kernel + user address spaces
    - 複製 ring buffer 內容到 shared section
    - 快取 mapping 以後續重用
  - `M487InterceptCleanup()` - 關閉 section handle
  - `M487InterceptInitialize()` - 初始化 section handle fields

- **`driver/M487FilterIoctl.h`** - 新增：
  - `IOCTL_M487FILTER_MAP_RING_BUFFER` (CTL_CODE)
  - `M487_FILTER_RING_MAPPING_INFO` structure:
    - `RingBufferUserVa`, `RingBufferSize`
    - `SlotSize`, `SlotCount`, `MaxDataSize`

- **`driver/M487FilterIoctl.c`** - 新增 IOCTL handler:
  - `IOCTL_M487FILTER_MAP_RING_BUFFER` handler

---

## 檔案變更摘要

| 檔案 | 變更 |
|------|------|
| `app/hidlog.c` | **新檔案** - CLI 工具 |
| `app/CMakeLists.txt` | 更新 - +hidlog target |
| `app/build.cmd` | **新檔案** - 建置腳本 |
| `driver/M487FilterIntercept.h` | +RingBufferSectionHandle/RingBufferUserVa, +M487InterceptMapRingBufferToUser() |
| `driver/M487FilterIntercept.c` | +Zw syscall 宣告, +M487InterceptMapRingBufferToUser() 實作, +cleanup updates |
| `driver/M487FilterIoctl.h` | +IOCTL_M487FILTER_MAP_RING_BUFFER, +M487_FILTER_RING_MAPPING_INFO |
| `driver/M487FilterIoctl.c` | +IOCTL_M487FILTER_MAP_RING_BUFFER handler |

---

## hidlog.exe 使用方式

### 基本用法
```powershell
# 顯示最近 16 筆 captured HID 封包
hidlog.exe

# 顯示最近 100 筆
hidlog.exe -n 100

# 只顯示 SET_FEATURE 封包
hidlog.exe --filter set-feature

# 只顯示 WRITE_REPORT 封包
hidlog.exe --filter write-report

# 持續監控新封包（tail -f 模式）
hidlog.exe -f

# 持續監控 SET_FEATURE
hidlog.exe -f --filter set-feature

# 只顯示統計資訊
hidlog.exe -s

# 安靜模式（純 hex）
hidlog.exe -q --filter write-report

# 禁用 ANSI 色彩
hidlog.exe --no-color
```

### 輸出範例
```
========================================
  hidlog - M487Filter HID Packet Log
========================================
  Ring Buffer: VA=0x0000012345678900  Size=18432 bytes
  Slots: 256 x 72 bytes  MaxData: 64 bytes
  Filter: all
========================================

[   1] 2026-03-28 14:30:01.234 WREP IsWrite=T Len=8 ReportId=0x01 IOCTL=0x000D010B
  0000: 01 02 03 04 05 06 07 08                          ........

[   2] 2026-03-28 14:30:01.456 GETF IsWrite=F Len=8 ReportId=0x01 IOCTL=0x000D010C
  0000: 00 00 00 00 00 00 00 00                          ........
```

---

## 技術架構

### User-Mode Ring Buffer Access
```
hidlog.exe                           M487Filter.sys
    │                                      │
    │  IOCTL_M487FILTER_MAP_RING_BUFFER    │
    │ ──────────────────────────────────> │
    │                                      │
    │   ZwCreateSection() [kernel]         │
    │   ZwMapViewOfSection(kernel)         │
    │   ZwMapViewOfSection(user)           │
    │   RtlCopyMemory(kernel→section)      │
    │                                      │
    │ < userVa, size, slot info ────────── │
    │                                      │
    │   [read section directly]            │
    │                                      │
    │   (polling: every 100ms in -f mode)  │
```

### Hexdump Format
```
  0000: 01 02 03 04 05 06 07 08  ........
  0008: 09 0A 0B 0C 0D 0E 0F 10  ........
```
- Offset (4 hex digits)
- 16 hex bytes (with extra space at byte 7)
- ASCII representation (. for non-printable)

### Color Coding
- 🔵 CYAN: Header / Stats
- 🟢 GREEN: SET_FEATURE (host → device)
- 🟡 YELLOW: GET_FEATURE (host ← device)
- 🔵 CYAN: WRITE_REPORT (host → device)
- 🟣 MAGENTA: READ_REPORT (host ← device)

---

## 待驗證

- [ ] Driver 端的 `IOCTL_M487FILTER_MAP_RING_BUFFER` handler 需要 WDK 編譯驗證
- [ ] `ZwCreateSection`/`ZwMapViewOfSection` 在 WDM driver 中的正確性
- [ ] User-mode 能否成功映射 ring buffer（需要實際設備）
- [ ] `-f` follow 模式在高頻 HID 流量下的效能

---

## T027f - WMI Interface 完善

> **任務狀態：** ✅ 已完成
> **日期：** 2026-03-28
> **依賴任務：** T027e（ETW + Ring Buffer 基礎）

---

## 完成項目

### 1. WMI GUID 衝突修復
- **`M487FilterMof.h`** - 新增 `M487FilterInterceptConfigWmi_GUID` 定義：
  - GUID: `{E5F6A7B8-9D8C-5A4B-C3D2-0F1E2A3B4C5D}`
  - 對應 `M487FilterInterceptConfigWmi` 結構
- **`M487FilterWmi.c`** - 第三個 WMI Provider (`EvtInterceptConfig*`)：
  - ✅ 修復：使用 `M487FilterInterceptConfigWmi_GUID` 而非 `M487FilterRingBufferInfo_GUID`
  - WMI Query Instance: `EvtInterceptConfigQueryInstance` - 讀取 4 個攔截開關
  - WMI Set Instance: `EvtInterceptConfigSetInstance` - 寫入所有配置
  - WMI Set Item: `EvtInterceptConfigSetItem` - 支援 WmiDataId(1-4) 個別設定

### 2. MOF 檔案完整化
- **`M487Filter.mof`** - 新增兩個 WMI Class：
  - **`M487FilterRingBufferInfo`** (GUID: `{D4E5F6A7-...}`)
    - RingBufferPhysicalAddress, RingBufferSize, TotalCaptured
    - DroppedCount, CurrentUsedSlots, TotalSlots, SlotSize, RingInitialized
    - 全部唯讀（由 driver 內核控制）
  - **`M487FilterInterceptConfigWmi`** (GUID: `{E5F6A7B8-...}`)
    - CaptureSetFeature, CaptureGetFeature (read/write)
    - CaptureWriteReport, CaptureReadReport (read/write)

### 3. User-mode WMI 查詢應用程式
- **`app/M487FilterWmiApp.c`** - 新完整功能：
  - 使用 Windows WMI COM API (IWbemServices/IWbemLocator)
  - **Query 功能**：
    - `--query-ring` / `-qr`: 查詢 ring buffer 狀態
    - `--query-config` / `-qc`: 查詢攔截配置
    - `--query-device` / `-qd`: 查詢設備資訊
    - `--query-all` / `-qa`: 查詢全部
  - **Set 功能**：
    - `--set-config [flags]`: 設定攔截配置（Enable/disable 個別開關）
    - 支援 `--enable-set-feature`, `--disable-get-feature` 等
  - **Monitor 功能**：
    - `--monitor` / `-m`: 持續監控 ring buffer 變化
  - **實作技術**：
    - COM初始化 + `CoInitializeSecurity`
    - `IWbemLocator::ConnectServer` → `root\wmi`
    - `IWbemServices::ExecQuery` (WQL) 查詢
    - `IWbemServices::PutInstance` (WBEM_FLAG_UPDATE_ONLY) 寫入
    - `Variant` 處理 + 類型轉換（VT_BOOL, VT_UI4, VT_UI8）
- **`app/CMakeLists.txt`** - CMake 建置腳本

### 4. 其他修復
- **`M487FilterIntercept.c`** - 移除 `M487CaptureEntry()` 中重複的 `M487RingBufferCommitEntry` 呼叫
- **`M487FilterMof.h`** - 新增 WMI DataItem ID defines（`CaptureSetFeature_ID` 等）

---

## 檔案變更摘要

| 檔案 | 變更 |
|------|------|
| `M487FilterMof.h` | +`M487FilterInterceptConfigWmi_GUID` 定義 |
| `M487FilterWmi.c` | +DataItem ID defines, ✅ Fix GUID collision |
| `M487Filter.mof` | +`M487FilterRingBufferInfo` + `M487FilterInterceptConfigWmi` Class |
| `M487FilterIntercept.c` | -移除 duplicate `M487RingBufferCommitEntry` |
| **`app/M487FilterWmiApp.c`** | 新檔案 - WMI Query/Set/Monitor 應用程式 |
| **`app/CMakeLists.txt`** | 新檔案 - CMake 建置 |

---

## WMI 使用方式

### 查詢 Ring Buffer 狀態
```powershell
# 使用 WMI 工具
wmic /namespace:\\root\wmi path M487FilterRingBufferInfo

# 使用 PowerShell
Get-WmiObject -Namespace root\wmi -Class M487FilterRingBufferInfo

# 使用我們的應用程式
M487FilterWmiApp.exe --query-ring
```

### 查詢/設定 Intercept Config
```powershell
# 查詢
wmic /namespace:\\root\wmi path M487FilterInterceptConfigWmi

# 啟用所有攔截
M487FilterWmiApp.exe --set-config --all

# 只啟用 SET_FEATURE 攔截
M487FilterWmiApp.exe --set-config --enable-set-feature

# 停用所有
M487FilterWmiApp.exe --set-config --none

# 持續監控
M487FilterWmiApp.exe --monitor
```

### User-mode App 架構
```
M487FilterWmiApp.exe
  │
  ├── InitWmi()
  │     CoInitializeEx()
  │     CoCreateInstance(CLSID_WbemLocator)
  │     IWbemLocator::ConnectServer("root\\wmi")
  │     CoSetProxyBlanket()
  │
  ├── QueryRingBufferInfo()  → SELECT * FROM M487FilterRingBufferInfo
  │     IWbemServices::ExecQuery(WQL)
  │     IEnumWbemClassObject::Next()
  │     IWbemClassObject::Get() → Variant解析
  │
  ├── QueryInterceptConfig() → SELECT * FROM M487FilterInterceptConfigWmi
  │     同上
  │
  ├── SetInterceptConfig()   → IWbemServices::PutInstance
  │     IWbemClassObject::Clone()
  │     IWbemClassObject::Put() × 4 properties
  │     IWbemServices::PutInstance( WBEM_FLAG_UPDATE_ONLY )
  │
  └── MonitorRingBuffer()    → 輪詢 + 差異顯示
```

---

## 待驗證

- [ ] WDK 編譯確認（符號完整性）
- [ ] Driver 安裝後 WMI Provider 是否正確註冊
- [ ] `wmic` 能否正確查詢 `M487FilterRingBufferInfo`
- [ ] `wmic` 能否設定 `M487FilterInterceptConfigWmi`
- [ ] User-mode app 在無 driver 時的錯誤處理

---

## T027e 任務記錄（已銜接）



> **任務狀態：** ✅ 已完成
> **日期：** 2026-03-28
> **依賴任務：** T027d（IOCTL 截獲實作）

---

## 完成項目

### 1. ETW Tracing 支援（Windows Event Tracing）
- **`M487FilterEtw.h`** - ETW 定義：
  - Provider GUID: `{A1B2C3D4-E5F6-4A5B-8C7D-9E0F1A2B3C4D}`
  - Control GUID: `{B2C3D4E5-F6A7-4B5C-9D8E-0F1A2B3C4D5E}`
  - 定義 ETW Flags（IOCTL/RING_BUFFER/INTERCEPT/ERROR/WMI）
  - 定義 ETW Level（LOGICAL/OPERATIONAL/VERBOSE/INFO/DEBUG）
  - Event ID 範圍：IOCTL(1-99), RingBuffer(100-199), Config(200-299), WMI(300-399), Error(900-999)
  - Logging 巨集：M487FILTER_ETW_LOG_IOCTL/RING/CONFIG/ERROR
- **`M487FilterEtw.c`** - ETW 實作：
  - `M487FilterEtwInitialize()` / `M487FilterEtwUninitialize()`
  - `EventRegister()` / `EventUnregister()` 流程
  - `M487FilterEtwEnableCallback()` - Session 啟用/停用回調
  - `EventWrite()` 輔助函式

### 2. WMI 介面讀取 Ring Buffer
- **`M487FilterMof.h`** - 新增 WMI 類別：
  - **`M487FilterRingBufferInfo`** (GUID: `{D4E5F6A7-8C9D-4B5A-B3C2-1E0F2A3B4C5D}`)
    - RingBufferPhysicalAddress, RingBufferSize
    - TotalCaptured, DroppedCount, CurrentUsedSlots
    - TotalSlots, SlotSize, RingInitialized
  - **`M487FilterInterceptConfigWmi`** - 攔截配置 WMI 類別
  - 擴展 `M487FilterDeviceInformation` - 新增 CaptureEnabled 欄位
- **`M487FilterWmi.c`** - 實作 3 個 WMI Provider：
  1. `M487FilterDeviceInformation` - 設備基本資訊（原有）
  2. **`EvtRingBufferQueryInstance`** - 查詢 Ring Buffer 狀態（新增）
  3. **`EvtInterceptConfigQueryInstance/SetInstance`** - 查詢/設定攔截配置（新增）
  - WMI Set 支援即時變更攔截開關（SET/GET/READ/WRITE_FEATURE）
  - 唯讀 Ring Buffer（由 IOCTL capture 機制寫入）

### 3. ETW 與現有程式碼整合
- **M487FilterIntercept.c** - 在以下位置加入 ETW Logging：
  - `M487CaptureEntry()` - 每次捕獲 IOCTL 時記錄（IOCTL + RingBuffer stats）
  - `M487InterceptSetConfig()` - 配置變更時記錄
- **M487FilterDevice.c** - DriverEntry/DeviceAdd 中：
  - 初始化時呼叫 `M487FilterEtwInitialize()`
  - Cleanup 時呼叫 `M487FilterEtwUninitialize()`
- **M487FilterWmi.c** - WMI 操作時記錄 ETW Event

---

## 檔案變更摘要

| 檔案 | 變更 |
|------|------|
| `M487Filter.h` | +include `M487FilterEtw.h` |
| `M487FilterDevice.c` | +`M487FilterEtwInitialize/Uninitialize()` 呼叫 |
| `M487FilterIntercept.c` | +ETW logging 於 capture/set_config |
| `M487FilterMof.h` | +`M487FilterRingBufferInfo`, `M487FilterInterceptConfigWmi`, +`CaptureEnabled` |
| `M487FilterWmi.c` | +RingBuffer WMI provider, +InterceptConfig WMI provider, +ETW logging |
| **`M487FilterEtw.h`** | 新檔案 - ETW 定義（GUID, Flags, Level, Event IDs, Macros）|
| **`M487FilterEtw.c`** | 新檔案 - ETW 實作（Register/EnableCallback/EventWrite）|
| `SOURCES` | +`M487FilterEtw.c` |
| `CMakeLists.txt` | +`M487FilterEtw.c` |

---

## ETW 架構

```
┌─────────────────────────────────────────────────────────────┐
│                    ETW Consumer Sessions                     │
│  (Perfmon, tracelog, WPP, or custom app with EventCallback) │
└────────────────────────────┬────────────────────────────────┘
                             │ EventWrite()
                             │
┌────────────────────────────▼────────────────────────────────┐
│              M487Filter.sys (ETW Provider)                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  EventRegister() → M487FilterEtwRegistrationHandle  │   │
│  │  M487FilterEtwEnableCallback() - Session enable     │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                              │
│  ETW Logging Points:                                         │
│  ★ M487CaptureEntry() → IOCTL captured to ring buffer      │
│  ★ M487InterceptSetConfig() → Intercept config changed     │
│  ★ M487FilterWmiInitialize() → WMI provider registered     │
│  ★ EvtWmiInstance*() → WMI queries/sets                    │
│  ★ Error conditions (IOCTL forward fail, ring full, etc.)  │
└─────────────────────────────────────────────────────────────┘
```

---

## WMI 架構

```
┌─────────────────────────────────────────────────────────────┐
│                    WMI Consumer (wmic, PowerShell)          │
└────────────────────────────┬────────────────────────────────┘
                             │
┌────────────────────────────▼────────────────────────────────┐
│              M487Filter.sys (WMI Provider)                   │
│                                                              │
│  1. M487FilterDeviceInformation (GUID: C8E5F3B0...)         │
│     ★ Active, PassThroughEnabled, CaptureEnabled           │
│     → EvtWmiInstanceQueryInstance / SetInstance / SetItem   │
│                                                              │
│  2. M487FilterRingBufferInfo (GUID: D4E5F6A7...)            │
│     ★ RingBufferPhysicalAddress, TotalCaptured             │
│     ★ DroppedCount, CurrentUsedSlots                        │
│     → EvtRingBufferQueryInstance                            │
│                                                              │
│  3. M487FilterInterceptConfigWmi                            │
│     ★ CaptureSetFeature, CaptureGetFeature                  │
│     ★ CaptureWriteReport, CaptureReadReport                │
│     → EvtInterceptConfigQueryInstance / SetInstance        │
└─────────────────────────────────────────────────────────────┘
```

---

## User-Mode 工具使用方式

### ETW Tracing
```powershell
# 啟動 ETW session 追蹤 M487Filter
tracelog -start -f M487Filter.etl -guid #A1B2C3D4-E5F6-4A5B-8C7D-9E0F1A2B3C4D -level 4 -flag 0xFFFFFFFF

# 等一段時間後停止
tracelog -stop M487Filter

# 轉換為可讀格式
tracefmt -o M487Filter.txt M487Filter.etl
```

### WMI Query
```powershell
# 查詢 Ring Buffer 狀態
wmic /namespace:\\root\wmi path M487FilterRingBufferInfo

# 查詢/設定 Intercept Config
wmic /namespace:\\root\wmi path M487FilterInterceptConfigWmi
wmic /namespace:\\root\wmi path M487FilterInterceptConfigWmi set CaptureSetFeature=true
```

---

## 待驗證

- [ ] WDK 編譯確認（符號完整性）- 需 WDK 環境
- [ ] ETW Session 能否正確接收 Events（需實際設備）
- [ ] WMI Query 能否正確讀取 Ring Buffer 狀態
- [ ] WMI Set 能否即時變更 Intercept Config
- [ ] tracelog / tracefmt 工具鏈測試

---

---

## T027h - 安裝程式/簽章（最終完成）

> **任務狀態：** ✅ 已完成
> **日期：** 2026-03-28
> **依賴任務：** T027g（hidlog.exe CLI 完成）

---

## 完成項目

### 1. INF 安裝資訊檔完善
- **`M487Filter.inf`** - 更新為 NTamd64 架構相容：
  - `Manufacturer` section: `NTamd64.10.0...16299`（對應 Windows 10 1709+ x64）
  - `SourceDisksFiles`: 加入 `M487Filter.cat = 1`
  - 所有 `NT$ARCH$` 替換為明確的 `NTamd64`
  - `Include = hidhidu.inf; Needs = HID_Install.NTamd64*` 系列
  - `CatalogFile=M487Filter.cat` 在 `[Version]` 區塊

### 2. 驅動程式簽章方案
- **`SIGNING.md`** - 完整驅動程式簽章指南：
  - **Method 1: Test Signing（測試簽章）**
    - `bcdedit /set testsigning on` 啟用測試模式
    - New-SelfSignedCertificate 建立自我簽署憑證
    - inf2cat 生成 catalog: `/os:10_x64`
    - signtool 簽署 catalog
  - **Method 2: EV Code Signing（正式環境）**
    - DigiCert/GlobalSign/Sectigo EV 憑證
    - USB Token 安裝
    - signtool + SHA256 + RFC 3161 timestamp
  - **Method 3: Microsoft Hardware Dev Center（長期正式）**
    - HLK 測試通過後微軟簽章
    - Windows Update 自動分發
  - 故障排除章節（常見錯誤碼與修復）

### 3. 安裝/解除安裝腳本
- **`install.bat`** - 互動式安裝腳本：
  - Admin 權限檢查
  - 自動偵測 M487 設備（devcon / PowerShell WMI）
  - 支援三種模式：
    - `install.bat` - 自動偵測模式
    - `install.bat test-sign` - 啟用測試簽章並安裝
    - `install.bat ev-sign [pfx] [pwd]` - EV 憑證安裝
  - 整合：pnputil 加入 Driver Store + 驅動更新
  - 子程式：`:enable-test-signing`, `:test-sign-driver`, `:ev-sign-driver`

- **`uninstall.bat`** - 解除安裝腳本：
  - Admin 權限檢查
  - `sc stop` + `sc delete` 移除服務
  - devcon / PowerShell `Remove-PnpDevice` 卸載設備
  - pnputil 移除 Driver Store 條目
  - 支援 `uninstall.bat full` 完整清除模式

### 4. 目錄結構最終狀態

```
M487Filter/
├── Task.md              # T027g + T027h 完成記錄
├── README.md            # 專案總覽
├── driver/
│   ├── M487Filter.sys       # 驅動程式本體（需 WDK 編譯）
│   ├── M487Filter.inf       # 安裝資訊檔（已完善 NTamd64）
│   ├── M487Filter.cat       # 簽署過的 Catalog（build 後產生）
│   ├── M487Filter.mof       # WMI MOF 定義
│   ├── M487Filter.rc        # 版本資源
│   ├── M487Filter.c         # DriverEntry
│   ├── M487Filter.h         # 主 Header（含所有模組）
│   ├── M487FilterDevice.h/c # 裝置上下文 + EvtDriverDeviceAdd
│   ├── M487FilterIoctl.h/c   # IOCTL handler + interception entry
│   ├── M487FilterIntercept.h/c # IOCTL 截獲實作
│   ├── M487FilterRingBuffer.h/c # Ring buffer 實作
│   ├── M487FilterWmi.h/c     # WMI providers
│   ├── M487FilterEtw.h/c     # ETW tracing
│   ├── M487FilterMof.h       # WMI GUID 定義
│   ├── SOURCES               # WDK build file
│   ├── CMakeLists.txt        # CMake build file
│   ├── makefile              # WDK makefile
│   ├── install.bat           # 安裝腳本（新增）
│   ├── uninstall.bat         # 解除安裝腳本（新增）
│   └── SIGNING.md            # 驅動程式簽章指南（新增）
└── app/
    ├── CMakeLists.txt        # App 建置設定
    ├── build.cmd              # 建置腳本
    ├── hidlog.c               # CLI 工具（T027g）
    └── M487FilterWmiApp.c     # WMI 查詢工具（T027f）
```

---

## 安裝流程

### 快速開始（測試簽章）
```powershell
# 1. 啟用測試簽章（需重開機）
bcdedit /set testsigning on

# 2. 重新開機後，編譯驅動程式
cd driver
build -g -km

# 3. 安裝驅動
install.bat test-sign

# 4. 連接 M487 設備
```

### 正式環境（EV 憑證）
```powershell
# 取得 EV Code Signing 憑證後
install.bat ev-sign "C:\certs\my-cert.pfx" "password"
```

### 解除安裝
```powershell
uninstall.bat
```

---

## 待驗證

- [ ] WDK 編譯確認（M487Filter.sys 產出）
- [ ] inf2cat 執行正確產生 M487Filter.cat
- [ ] 測試簽章驅動程式成功載入（bcdedit testsigning on 後）
- [ ] EV 憑證安裝流程驗證
- [ ] 設備插拔後 driver 正確自動綁定
- [ ] hidlog.exe -f 模式正常運作
- [ ] WMI Query/Set 正確運作

---

## 參考文獻

- [ETW in Kernel-Mode Drivers (MSDN)](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/event-tracing-for-windows)
- [WDF WMI Support (MSDN)](https://docs.microsoft.com/en-us/windows-hardware/drivers/wdf/wmi-support-in-umdf-drivers)
- [Windows Driver Kit (WDK)](https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk)
- [Driver Signing (Microsoft Docs)](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/driver-signing)
- [SignTool (Microsoft Docs)](https://docs.microsoft.com/en-us/windows/win32/seccrypto/signtool)
- [Inf2Cat (Microsoft Docs)](https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/inf2cat)
