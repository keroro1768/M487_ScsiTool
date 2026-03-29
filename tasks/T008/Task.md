# Task.md — T008

## 基本資訊

| 欄位 | 內容 |
|------|------|
| ID | T008 |
| 標題 | BSP HSUSBD 框架整合（T004 USB HID Layer）|
| 狀態 | in-progress |
| 優先序 | P1 |
| 指派 | Giroro |
| 依賴 | T004 Phase 3 完成 |
| 截止 | - |

## 目標

將 T004 的 USB HID Layer Stub 整合進 BSP HSUSBD 框架

## 需求

- [x] 研究 BSP HSUSBD_HID_Transfer_And_MSC 範例
- [ ] 整合 HID Class Request Handler
- [ ] 整合 EP0 Control Endpoint
- [ ] 整合 EP1/EP2 Interrupt
- [ ] 整合 USBD20_IRQHandler
- [ ] 編譯驗證
- [ ] 燒錄測試（需硬體）

## 進度

研究完成。關鍵發現：BSP 使用 gsHSInfo 結構，需將 bridge callback 整合進去。

## 備註

複雜整合工作，需要 BSP 庫支援。建議採用 BSP 範例為基礎修改。
