# T027c - M487 HID Filter Driver 骨架開發

## 任務狀態: ⏳ 待執行

## 母任務
T027 - M487 HID Filter Driver 開發

## 前置條件
- ✅ T027b Firefly 範本研究完成

## 任務目標
基於 T027b 的 Firefly HID Filter Driver 研究結果，建立 M487 HID Filter Driver 的程式碼骨架。

## 已知資訊（來自 T027b 研究）

### 參考架構
- **Framework:** KMDF (Kernel Mode Driver Framework)
- **過濾位置:** Upper Filter Driver（HID 裝置堆疊上方）
- **控制介面:** WMI（user-mode ↔ kernel-mode）
- **I/O 目標:** `WdfIoTarget` + `IOCTL_HID_GET/SET_FEATURE`

### 目標硬體
- **VID/PID:** （待確認 - 需從 M487 BSP 或實際裝置取得）
- **通訊協議:** USB HID（Feature Report 格式待確認）

### 需要建立的檔案（預估）

```
T027c/
├── Task.md                    # 本檔案
├── driver/
│   ├── M487HidFilter.c        # DriverEntry + WDF_DRIVER_CONFIG
│   ├── M487HidFilter.h        # 主要 header
│   ├── device.c               # EvtDeviceAdd + WdfFdoInitSetFilter
│   ├── device.h               # DEVICE_CONTEXT
│   ├── vfeature.c             # IOCTL_HID_SET/GET_FEATURE 處理
│   ├── vfeature.h             # Feature report 函式宣告
│   ├── wmi.c                  # WMI 初始化 + handlers
│   ├── wmi.h                  # WMI 函式宣告
│   ├── M487HidFilter.inf      # INF 安裝檔（UpperFilters）
│   └── M487HidFilter.mof      # WMI class 定義
├── app/
│   └── M487HidTool/          # User-mode 控制工具（參考 flicker.exe）
└── README.md
```

## 實作步驟

### Phase 1: 驅動程式骨架
1. 建立 KMDF 專案結構（基於 firefly 範本）
2. 修改 `EvtDeviceAdd` 中的 `WdfFdoInitSetFilter()` 設定
3. 確認目標 VID/PID（HID\Vid_xxxx&Pid_yyyy）
4. 確認 UpperFilters registry 註冊

### Phase 2: Feature Report 處理
1. 參考 `vfeature.c` 建立 IOCTL 發送函式
2. 確認 M487 的 HID Report Descriptor
3. 確認 Feature Report 格式（Usage Page / Usage ID）
4. 實作 `IOCTL_HID_SET_FEATURE` / `IOCTL_HID_GET_FEATURE` 處理

### Phase 3: WMI 介面
1. 定義 M487 WMI GUID（可參考 firefly `{AB27DB29-DB25-42E6-A3E7-28BD46BDB666}`）
2. 實作 WMI class + instance registration
3. 實作 `EvtWmiInstanceSetInstance` / `EvtWmiInstanceQueryInstance`
4. 建立 user-mode 控制工具

## 待確認資訊

| 項目 | 狀態 | 說明 |
|------|------|------|
| M487 USB HID VID/PID | ❓ 待確認 | 需從 BSP 或實際枚舉確認 |
| M487 Feature Report 格式 | ❓ 待確認 | Usage Page/ID、資料結構 |
| M487 Report Descriptor | ❓ 待確認 | 需從 M487 HID 描述符分析 |
| WMI GUID | ❓ 待定義 | 需新建 GUID |
| 安裝方式 | ❓ 待確認 | 可能需要 Zadig 或手動 INF |

## 依賴
- T027b 研究成果（Firefly 程式碼模式）
- M487 BSP USB HID 實作細節
- 實際 M487 裝置（用於測試INF VID/PID）

## 建立時間
2026-03-28 12:26 GMT+8
