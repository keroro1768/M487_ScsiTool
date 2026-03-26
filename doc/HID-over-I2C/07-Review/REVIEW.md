# HID-over-I²C Bridge — Review Checklist & Report Template

> Branch: `architecture/hid-over-i2c`

---

## 1. Review Policy

**每個階段完成後都必須進行 Review，不得跳過。**

Review 不是可選的 end-of-project 活動，而是：
- 每完成一個實作單元 → 立即 Review
- 階段測試完成後 → 立即 Review
- 整個專案完成前 → 最終 Review

---

## 2. Review 等級

### 2.1 程式碼 Review（Code Review）
- 適用：每個函式/模組實作完成時
- 目的：確保程式碼品質、找出錯誤、確認一致性
- 方式：自我檢查 + 對照 SPEC.md 規格

### 2.2 整合 Review（Integration Review）
- 適用：多個模組整合後
- 目的：確認介面正確互動、沒有契約破壞
- 方式：對照 ARCHITECTURE.md 層級定義

### 2.3 最終 Review（Final Review）
- 適用：整個專案完成，準備交付前
- 目的：確認所有規格達成、文件完整、測試覆蓋
- 方式：對照 SPEC.md + TEST_PLAN.md 逐項檢查

---

## 3. Review 檢查清單

### 3.1 規格一致性

| # | 檢查項目 | 依據 | 狀態 |
|---|---------|------|------|
| S-01 | HID Descriptor 格式符合 spec（30 bytes, bcdVersion=0x0100） | SPEC.md §3 | ☐ |
| S-02 | Register indices 不為零且唯一 | SPEC.md §3 | ☐ |
| S-03 | Command Register 格式正確（Opcode + ReportType + ReportID） | SPEC.md §5 | ☐ |
| S-04 | Data Register 包含 2-byte length prefix | SPEC.md §4 | ☐ |
| S-05 | GET_REPORT 流程：Write Command → Read Data | SPEC.md §6 | ☐ |
| S-06 | SET_REPORT 流程：Write Data → Write Command | SPEC.md §6 | ☐ |
| S-07 | RESET command 格式正確 | SPEC.md §6 | ☐ |
| S-08 | Input Report interrupt-driven delivery | SPEC.md §2 | ☐ |
| S-09 | Report ID encoding（≥15 時使用 sentinel） | SPEC.md §14 | ☐ |
| S-10 | Error handling 符合 spec（timeout, NACK, STALL） | SPEC.md §12 | ☐ |

### 3.2 架構一致性

| # | 檢查項目 | 依據 | 狀態 |
|---|---------|------|------|
| A-01 | USB 為 HID Device（非 Host） | ARCHITECTURE.md §1 | ☐ |
| A-02 | EP0 Control + EP1 Interrupt IN + EP2 Interrupt OUT | ARCHITECTURE.md §4.2 | ☐ |
| A-03 | 所有 HID Class Requests 皆有實作 | ARCHITECTURE.md §4.4 | ☐ |
| A-04 | 傳輸層正確映射 USB→I²C 請求 | ARCHITECTURE.md §5.1 | ☐ |
| A-05 | I²C 速度與 HID-over-I²C spec 相符 | ARCHITECTURE.md §3 | ☐ |
| A-06 | GPIO interrupt 連接到 I²C device | ARCHITECTURE.md §2 | ☐ |
| A-07 | HID Descriptor 有被快取 | ARCHITECTURE.md §6 | ☐ |

### 3.3 實作品質

| # | 檢查項目 | 標準 | 狀態 |
|---|---------|------|------|
| Q-01 | 程式碼有註解，函式有 docstring | - | ☐ |
| Q-02 | 錯誤回傳值一致（-1 = error, 0 = ok） | - | ☐ |
| Q-03 | 所有 buffer 有邊界檢查 | - | ☐ |
| Q-04 | I²C transaction 有 timeout | - | ☐ |
| Q-05 | NACK 有 retry 機制（最多3次） | - | ☐ |
| Q-06 | 函式不超過 200 行 | - | ☐ |
| Q-07 | 沒有 hardcoded magic numbers | - | ☐ |
| Q-08 | 使用 defined constants 而非 raw values | - | ☐ |
| Q-09 | volatile 用於 ISR 共享變數 | - | ☐ |
| Q-10 | Critical section 有中斷遮蔽或 disable/enable | - | ☐ |

### 3.4 測試覆蓋

| # | 檢查項目 | 依據 | 狀態 |
|---|---------|------|------|
| T-01 | I²C init → Unit test passed | TEST_PLAN.md §2.1 | ☐ |
| T-02 | HID Descriptor parser → Unit test passed | TEST_PLAN.md §2.2 | ☐ |
| T-03 | Command builder → Unit test passed | TEST_PLAN.md §2.3 | ☐ |
| T-04 | USB enumeration → Integration test passed | TEST_PLAN.md §3.1 | ☐ |
| T-05 | GET_REPORT → Integration test passed | TEST_PLAN.md §3.2 | ☐ |
| T-06 | SET_REPORT → Integration test passed | TEST_PLAN.md §3.2 | ☐ |
| T-07 | Interrupt handling → Integration test passed | TEST_PLAN.md §3.3 | ☐ |
| T-08 | Error handling (NACK, timeout) → System test passed | TEST_PLAN.md §4.2 | ☐ |

### 3.5 文件完整性

| # | 檢查項目 | 狀態 |
|---|---------|------|
| D-01 | README.md 文件索引完整 | ☐ |
| D-02 | SPEC.md 所有章節皆已填寫 | ☐ |
| D-03 | ARCHITECTURE.md 與實作一致 | ☐ |
| D-04 | TRANSLATION.md 流程圖與實作一致 | ☐ |
| D-05 | REGISTERS.md 常數定義與程式碼一致 | ☐ |
| D-06 | EXAMPLE.md 範例可實際執行 | ☐ |
| D-07 | TEST_PLAN.md 測試項目皆已完成或標記 Waived | ☐ |

---

## 4. Review 報告格式

### 4.1 Header

```
# [專案名稱] Review 報告
- 日期：[YYYY-MM-DD]
- Review 等級：[程式碼/整合/最終]
- Review 人員：[名稱]
- 階段：[實作/測試/交付前]
```

### 4.2 摘要

```
## 摘要
通過：[X]/[Y] 檢查項目
失敗：[N]/[Y] 檢查項目
風險：[M] 項
建議：[Z] 項
整體評估：[Pass/Conditional Pass/Fail]
```

### 4.3 失敗項目

```
## 失敗項目（需修復）

### [ID] [失敗項目名稱]
- **檢查依據：** [規格文件章節]
- **問題描述：** [詳細說明]
- **嚴重性：** [Critical/Major/Minor]
- **建議修復方式：** [具體建議]
```

### 4.4 風險項目

```
## 風險項目（監控中）

### [ID] [風險名稱]
- **風險描述：** [說明]
- **潛在影響：** [影響]
- **緩解措施：** [措施]
```

### 4.5 建議事項

```
## 建議事項

### [ID] [建議名稱]
- **建議理由：** [說明]
- **預期效益：** [效益]
- **優先度：** [High/Medium/Low]
```

---

## 5. Review 流程

```
實作完成
    │
    ▼
程式碼自我檢查（對照檢查清單 §3.3）
    │
    ▼
單元測試（§3.4 T-01～T-03）
    │
    ▼
整合測試（§3.4 T-04～T-07）
    │
    ▼
程式碼 Review Meeting
    │  找出失敗項目 → 修復 → 回到整合測試
    ▼
問題修復（§3.3 失敗項目）
    │
    ▼
文件 Review（§3.5）
    │
    ▼
最終 Review（§3.1～§3.5 全部）
    │
    ▼
發布 Review 報告
    │  有 Critical/Major 失敗 → 修復後重新 Review
    ▼
交付
```

---

## 6. 通過標準

| 等級 | 通過標準 |
|------|---------|
| 程式碼 Review | §3.3 全部通過（Q-01～Q-10），無 Critical 問題 |
| 整合 Review | §3.2 全部通過（A-01～A-07），§3.4 T-04～T-07 通過 |
| 最終 Review | §3.1～§3.5 全部通過，無 Critical 或 Major 問題 |

**注意：** Minor 問題不阻礙交付，但需在報告中說明並列入追蹤。
