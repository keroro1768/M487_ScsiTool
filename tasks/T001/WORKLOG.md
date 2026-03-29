# WORKLOG.md — T001

## 工作日誌

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
| 下午 | GCC 編譯環境建立，韌體編譯成功（43.9KB）| Giroro |
| 下午 | 燒錄驗證成功（46784 bytes verified）| Giroro |

### 2026-03-27

| 時間 | 工作內容 | 負責 |
|------|---------|------|
| 全日 | 程式碼 Review（T011）發現多項問題 | Dororo + Giroro |
| 全日 | NACK Retry、統一錯誤碼、Buffer 邊界檢查修復 | Giroro |
| 下午 | T008 研究：BSP HSUSBD 框架分析 | Giroro |

### 問題記錄

- OpenOCD LIBUSB_ERROR_ACCESS：驅動程式問題，Zadig 置換後仍無法連線
- I2C_Read() 函式尚未完整實作

### Commit

- `265cb4c` - Reorganize: unify workspace, add tasks/docs, new debug modules
- `e6a5c37` - T001 firmware initial commit
