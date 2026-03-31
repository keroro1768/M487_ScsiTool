# WORKLOG.md — T001

## 工作日誌

### 2026-03-31

| 時間 | 工作內容 | 負責 |
|------|---------|------|
| 上午 | OpenOCD 燒錄失敗排查：`nulink_m487_ice.cfg` 遺失 `hla layout nulink` | Giroro |
| 上午 | 修復 cfg：加回 `hla layout nulink`，驗證連線成功 | Giroro |
| 上午 | ICE 文件全面更新（QUICK_START / 02_SOLUTION / 04_VERIFICATION）| Giroro |
| 上午 | 確認 I2C_Read() 已完整實作（i2c_control.c + hid_i2c.c）| Giroro |
| 上午 | 更新 T001 Task.md、WORKLOG.md 反映真實進度 | Giroro |

### 2026-03-28

| 時間 | 工作內容 | 負責 |
|------|---------|------|
| 全日 | Task 資料夾結構重構（套用新格式）| Giroro |
| 全日 | 所有 Task VERIFY.md 建立/更新 | Giroro |
| 全日 | TaskList.md 更新 | Giroro |

### 2026-03-26

| 時間 | 工作內容 | 負責 |
|------|---------|------|
| 上午 | 韌體架構設計（MSC + HID I2C Bridge）| Giroro |
| 下午 | GCC 編譯環境建立，韌體編譯成功（43.9KB → 61.2KB）| Giroro |
| 下午 | 燒錄驗證成功（46784 bytes verified）| Giroro |

### 2026-03-27

| 時間 | 工作內容 | 負責 |
|------|---------|------|
| 全日 | 程式碼 Review（T011）發現多項問題 | Dororo + Giroro |
| 全日 | NACK Retry、統一錯誤碼、Buffer 邊界檢查修復 | Giroro |
| 下午 | T008 研究：BSP HSUSBD 框架分析 | Giroro |

### 2026-03-30

| 時間 | 工作內容 | 負責 |
|------|---------|------|
| 全日 | OpenOCD + Nu-Link ICE 連線解決（重大突破）| Giroro |
| 全日 | VSCode F5 Debug 驗證成功 | Giroro |
| 下午 | GDB Debug 完整功能驗證（斷點/單步/記憶體/Watchpoint）| Giroro |
| 下午 | ICE 文件整理（doc/ICE/ 6 份文件）| Giroro |

---

## 問題記錄

| 日期 | 問題 | 狀態 | 解決方式 |
|------|------|------|---------|
| 2026-03-26 | OpenOCD LIBUSB_ERROR_ACCESS | ✅ 已解決 | 使用 WinUSB + hla driver |
| 2026-03-30 | CMSIS-DAP vs Nu-Link 驅動不符 | ✅ 已解決 | 換用 hla layout nulink |
| 2026-03-31 | OpenOCD 燒錄失敗（cfg 遺失 hla layout） | ✅ 已解決 | 修復 nulink_m487_ice.cfg |
| 2026-03-26 | I2C_Read() 函式尚未完整實作 | ✅ 已解決 | i2c_control.c 完整實作含 NACK retry |

---

## Commit

- `265cb4c` - Reorganize: unify workspace, add tasks/docs, new debug modules
- `e6a5c37` - T001 firmware initial commit
