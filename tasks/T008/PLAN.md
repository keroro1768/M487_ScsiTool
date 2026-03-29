# PLAN.md — T008

## 執行計畫

### Step 1：以 BSP 範例為基礎修改
`D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\HSUSBD_HID_Transfer_And_MSC`

### Step 2：置換 HID Handler 為 Bridge Callback
- HID_Init → bridge_init
- EPA_Handler → input report
- EPB_Handler → output report

### Step 3：整合 gsHSInfo Descriptors
- 確認 HID Descriptor 正確
- 確認 EP 配置

### Step 4：編譯 + 燒錄驗證

## 預估工時

4-6 hr
