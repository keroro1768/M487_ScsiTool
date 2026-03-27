# HID-over-I2C Bridge - 測試計畫

## 測試環境

- **目標晶片**：Nuvoton M487 (Cortex-M4)
- **燒錄器**：Nu-Link 或相容 OpenOCD 燒錄器
- **主機工具**：`hidtool`（參見 `tool/hidtool/SPEC.md`）
- **測試目標**：HID-over-I2C Bridge 功能驗證

---

## Phase 1：環境驗證（硬體基礎確認）

### P1.1 BSP 編譯驗證
- [ ] M480 BSP 可正常編譯（使用 arm-none-eabi-gcc）
- [ ] 標準驅動程式（CLK, GPIO, SYS）可正常初始化

### P1.2 最小系統驗證
- [ ] 晶片可透過 OpenOCD 燒錄
- [ ] UART 除錯輸出正常運行（115200 baud）
- [ ] 系統時脈正確配置（HXTAL 12MHz）

---

## Phase 2：USB HID STUB 驗證

### P2.1 USB 列舉
- [ ] 裝置插入 USB 後，主機可正確識別為 HID 裝置
- [ ] USB 描述符正確（需先實作完整的 usb_hid.c）
- [ ] VID/PID 與描述符匹配

### P2.2 端點通訊
- [ ] Interrupt IN 端點可發送資料至主機
- [ ] Interrupt OUT 端點可接收來自主機的資料
- [ ] 控制傳輸（GET_REPORT, SET_REPORT）正常處理

### P2.3 USB 中斷處理
- [ ] USB bus reset 正確處理
- [ ] suspend/resume 正確處理
- [ ] VBUS detect 正確運作

---

## Phase 3：I2C Master 驗證

### P3.1 I2C 基礎通訊
- [ ] I2C bus 可正確初始化（100kHz / 400kHz）
- [ ] I2C master 可正確發送 START/STOP
- [ ] I2C master 可正確發送位址 + 讀寫位元

### P3.2 I2C 讀寫
- [ ] 單一位元組寫入正確
- [ ] 多位元組寫入正確
- [ ] 單一位元組讀取正確
- [ ] 多位元組讀取正確

### P3.3 I2C 錯誤處理
- [ ] NAK 正確處理（重試或錯誤回報）
- [ ] Bus error 正確處理
- [ ] Timeout 正確處理

---

## Phase 4：Bridge 層驗證

### P4.1 HID Report 解析
- [ ] 正確解析 host 發送的 HID Input report
- [ ] 正確解析 host 發送的 HID Feature report
- [ ] Report ID 解析正確（如使用 report ID）

### P4.2 I2C ↔ HID 轉發
- [ ] Host 發送的 HID report 正確轉發至 I2C bus
- [ ] I2C read 回應正確封裝為 HID report 發送至 host
- [ ] 轉發延遲在可接受範圍內（< 10ms）

### P4.3 邊界情況
- [ ] I2C NAK 時，正確回傳錯誤至 host
- [ ] I2C timeout 時，正確回傳錯誤至 host
- [ ] 過長的 HID report 正確截斷或拒絕

---

## Phase 5：整合測試

### P5.1 功能測試
- [ ] 完整流程：host 發送 HID report → M487 解析 → I2C 寫入 → 回應 → host 收到回覆
- [ ] 完整流程：M487 I2C 讀取 → 封裝 HID report → host 收到資料
- [ ] Hotplug：拔除後重新插入，裝置重新列舉

### P5.2 效能測試
- [ ] 最大 HID report 傳輸速率
- [ ] I2C 100kHz 模式吞吐量
- [ ] I2C 400kHz 模式吞吐量
- [ ] 端對端延遲測量

### P5.3 穩定性測試
- [ ] 長時間運行測試（24 小時）
- [ ] 大量資料傳輸測試（連續 10000 次交易）
- [ ] 錯誤恢復驗證（模擬 I2C NAK、bus reset）

---

## 測試工具需求

### hidtool（主機端）
```
hidtool device list                          # 列出已連接的 HID 裝置
hidtool read <i2c_addr> <reg> [len]          # 讀取 I2C 暫存器
hidtool write <i2c_addr> <reg> <data>...    # 寫入 I2C 暫存器
hidtool log [on|off|export]                  # 除錯日誌控制
```

### 測試用 I2C 從屬裝置
- **建議**：使用 I2C EEPROM（如 24LC256）作為測試從屬
- 可驗證讀寫一致性

---

## 失敗處理策略

| 測試項目 | 失敗跡象 | 排查方向 |
|---------|---------|---------|
| USB 列舉失敗 | 裝置嘆號 | 檢查描述符、endpoints 配置 |
| I2C 無回應 | timeout | 檢查 SDA/SCL 線路、使用示波器確認時脈 |
| Bridge 轉發失敗 | log 顯示 decode error | 檢查 HID report format、byte order |
| 燒錄失敗 | OpenOCD error | 檢查燒錄器連線、目標晶片供電 |
