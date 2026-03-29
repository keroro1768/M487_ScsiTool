# Task.md — T004

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T004 |
| 標題 | HID-over-I2C Bridge 實作（完整軟體方案）|
| 狀態 | in-progress |
| 優先序 | - |
| 指派 | Giroro |
| 依賴 | T001 |
| 截止 | - |

## 目標

M487 作為標準 USB HID Device，Bridge 到 HID-over-I2C 協定的 I2C 裝置

## 需求

- [x] Phase 1: I2C 驅動實作
- [x] Phase 2: HID Descriptor Parser
- [x] Phase 3: USB HID Device Layer（需整合 T008）
- [x] Phase 4: 翻譯層實作
- [x] Phase 5: 整合與編譯
- [ ] USB HID Layer 完整實作（目前為 Stub）

## 進度

Phase 1-5 完成（編譯成功 43.3KB）。USB HID Layer 為 Stub，需 BSP HSUSBD 框架整合。

## 備註

待整合 T008（BSP HSUSBD 框架）
