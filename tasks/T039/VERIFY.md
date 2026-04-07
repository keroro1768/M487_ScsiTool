# T039 文件完整性驗證報告

> **驗證人：** Dororo（驗證/測試專員）
> **驗證日期：** 2026-04-07
> **專案：** M487_ScsiTool
> **任務：** 文件完整性驗證

---

## 📋 摘要

本次共驗證 **13 份文件**，涵蓋 IDE 設定、M487 使用指南、燒錄工具說明、HID-over-I2C 文件體系、以及任務模板。

| 驗證結果 | 文件數 | 狀態 |
|---------|--------|------|
| ✅ 資料正確、來源可靠、內容完整 | 7 | 通過 |
| ⚠️ 大致正確，需補充或修正 | 6 | 需修正 |
| ❌ 資料錯誤或來源不可靠 | 0 | - |

---

## 📊 驗證結果總表

| # | 文件 | 大小 | 技術準確 | 來源確認 | 完整性 | 連結有效 | 格式一致 | 錯字/語法 | 總評 |
|---|------|------|----------|----------|--------|----------|----------|-----------|------|
| 1 | VSCode_OpenOCD_GDB_M487_IDE_Setup.md | 15.7KB | ✅ | ✅ | ✅ | ⚠️ | ✅ | ✅ | ⚠️ |
| 2 | M487_Correct_Usage_Guide.md | 7.5KB | ✅ | ✅ | ✅ | ⚠️ | ✅ | ✅ | ⚠️ |
| 3 | NuLink_Experience_Compilation.md | 8.5KB | ✅ | ✅ | ✅ | ⚠️ | ⚠️ | ✅ | ⚠️ |
| 4 | HID-over-I2C/README.md | 2.8KB | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 5 | HID-over-I2C/01-Spec/SPEC.md | 8.8KB | ✅ | ⚠️ | ✅ | ⚠️ | ✅ | ✅ | ⚠️ |
| 6 | HID-over-I2C/02-Architecture/ARCHITECTURE.md | 11KB | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 7 | HID-over-I2C/03-Protocol/TRANSLATION.md | 17.6KB | ⚠️ | ✅ | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| 8 | HID-over-I2C/04-Registers/REGISTERS.md | 7.9KB | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 9 | HID-over-I2C/05-Example/EXAMPLE.md | 4.8KB | ⚠️ | ⚠️ | ⚠️ | ✅ | ✅ | ✅ | ⚠️ |
| 10 | HID-over-I2C/06-TestPlan/TEST_PLAN.md | 7.0KB | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 11 | HID-over-I2C/07-Review/REVIEW.md | 6.6KB | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 12 | Task_Template.md | 489B | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| 13 | Verify_Template.md | 412B | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

---

## 詳細驗證結果

---

### 1. VSCode_OpenOCD_GDB_M487_IDE_Setup.md ✅ → ⚠️

**路徑：** `doc/IDE_Setup/VSCode_OpenOCD_GDB_M487_IDE_Setup.md`
**大小：** 15.7KB

#### ✅ 通過項目
- **技術準確性：** 所有技術陳述均正確。OpenOCD 設定、`hla` driver、config 檔案內容、VSCode 設定語法、Cortex-Debug 欄位名稱皆與官方文件一致
- **來源確認：** xpack GCC、OpenOCD-Nuvoton GitHub、Cortex-Debug GitHub、Nuvoton VSCode Extension marketplace URL 皆為可靠來源
- **完整性：** Step 1-7 覆蓋完整，含編譯、燒錄、Debug、FAQ
- **格式一致性：** 表格、程式碼區塊、標題層級一致
- **錯字/語法：** 無

#### ⚠️ 需修正

**問題 1：連結驗證 — Quick Start 參考路徑可能失效**
- **位置：** 文件末尾
  ```
  快速上手見：[doc/ICE/QUICK_START.md](../ICE/QUICK_START.md)
  ```
- **說明：** 此相對路徑從 `doc/IDE_Setup/` 出發，需向上兩層到 `doc/`，再進入 `ICE/`。路徑語法理論上正確 (`../../../ICE/QUICK_START.md`)，但 `doc/ICE/QUICK_START.md` 檔案是否存在需確認
- **建議：** 將 `../ICE/QUICK_START.md` 改為 `../../ICE/QUICK_START.md`，或直接寫絕對路徑

**問題 2：版本標記**
- **說明：** 文件標記「2026-03-30（根據已驗證的 ICE 連線方案更新）」，建議確認是否需要更新版本日期

---

### 2. M487_Correct_Usage_Guide.md ✅ → ⚠️

**路徑：** `doc/M487/M487_Correct_Usage_Guide.md`
**大小：** 7.5KB

#### ✅ 通過項目
- **技術準確性：** M487 晶片規格（ARM Cortex-M4F、192MHz、512KB Flash、160KB SRAM）、USB HSUSBD 架構、BSP 驅動分層正確
- **來源確認：** Nuvoton 官網、GitHub BSP、AWS FreeRTOS 文件皆可靠
- **完整性：** 涵蓋晶片概述、USB 架構、BSP 範例、开发流程、常見錯誤、參考文件
- **格式一致性：** 表格、程式碼區塊、標題一致
- **錯字/語法：** 無

#### ⚠️ 需修正

**問題 1：連結驗證**
- Nuvoton 官網下載連結 (`https://www.nuvoton.com/...`) 無法現場確認，建議另存備份或使用 GitHub BSP 作為主要來源
- `https://www.nuvoton-mcu.com/userupload/TRM_M480_Series_SC_Rev1.00.pdf` 為第三方託管 URL，建議改用 Nuvoton 官方途徑下載

**問題 2：韌體檔案路徑假設**
- 文件假設 BSP 位於 `D:\AiWorkSpace\KM\M480BSP`，此路徑為本機環境專屬，非通用路徑。建議在文件開頭說明「此路徑為文件作者的本地環境路徑，讀者需替換為自己的 BSP 路徑」

---

### 3. NuLink_Experience_Compilation.md ✅ → ⚠️

**路徑：** `doc/NuLink/NuLink_Experience_Compilation.md`
**大小：** 8.5KB

#### ✅ 通過項目
- **技術準確性：** Nu-Link VID/PID (0x0416/0x511C)、驅動架構、Interface 0/1 分離、HLA driver、WinUSB 驅動安裝流程皆正確
- **來源確認：** GitHub Issues、Nuvoton 論壇、StackOverflow、OpenOCD 官方文件皆可靠
- **完整性：** 涵蓋型號對照、驅動架構、除錯流程、替代工具、推薦工作流程
- **錯字/語法：** 無

#### ⚠️ 需修正

**問題 1：內部連結路徑不一致**
- **位置：** 文件末尾
  ```
  M487 ICE 解決方案：[doc/ICE/QUICK_START.md](../ICE/QUICK_START.md)
  ICE 問題分析：[doc/ICE/01_PROBLEM.md](../ICE/01_PROBLEM.md)
  ```
- **說明：** 從 `doc/NuLink/` 目錄到 `doc/ICE/` 的正確相對路徑是 `../../ICE/`，不是 `../ICE/`。`../` 只會到 `doc/` 而非 `doc/ICE/`。實際 URL 會變成 `doc/NuLink/../ICE/` = `doc/ICE/`，湊巧結果正確，但語法冗餘不正確
- **建議：** 改為 `../../ICE/QUICK_START.md` 和 `../../ICE/01_PROBLEM.md`，以確保路徑解析正確

**問題 2：LIBUSB_ERROR_ACCESS 說明不完整**
- 當驅動正確但仍出現 `LIBUSB_ERROR_ACCESS` 時，需確認 MSYS2 DLL PATH 與 Zadig 設定的先後順序，說明可以更清楚

---

### 4. HID-over-I2C/README.md ✅

**路徑：** `doc/HID-over-I2C/README.md`
**大小：** 2.8KB

- **技術準確性：** ✅ 架構圖、系統描述正確
- **來源確認：** ✅ 純內部文件，無外部依賴
- **完整性：** ✅ 涵蓋文件索引、快速參考、狀態追蹤、實作路線圖
- **連結有效：** ✅ 所有內部 `.md` 連結皆為同目錄或子目錄，URL 正確
- **格式一致性：** ✅ 清晰的 emoji 標記、表格一致
- **錯字/語法：** ✅ 無

---

### 5. HID-over-I2C/01-Spec/SPEC.md ✅ → ⚠️

**路徑：** `doc/HID-over-I2C/01-Spec/SPEC.md`
**大小：** 8.8KB（目錄清單顯示 8,782B）

#### ✅ 通過項目
- **技術準確性：** HID Descriptor (30 bytes)、各欄位偏移量與大小、Register Model、Command Protocol opcode table、Report Protocol、Power Management、I²C Bus Parameters 皆與 Microsoft HID over I2C Protocol Specification v1.0 一致
- **完整性：** ✅ 涵蓋 Overview、System Architecture、HID Descriptor、Register Model、Command Protocol、Report Protocol、Interrupt Handling、Enumeration Flow、Power Management、Error Handling、I²C Parameters
- **格式一致性：** ✅ 表格格式一致
- **錯字/語法：** ✅ 無

#### ⚠️ 需修正

**問題 1：來源檔案路徑需驗證**
- **位置：** 文件開頭
  ```
  Location: D:\AiWorkSpace\KM\KM-collect\Hardware\I3C-USB-Bridge\Microsoft-HID-over-I2C-spec-full.md
  ```
- **說明：** 此為本地檔案路徑，無法現場驗證其存在性與內容完整性。建議於文件內說明此為參考來源的本地副本，並附上 Microsoft 官方下載連結作為備查
- **建議：** 加上 Microsoft 官方規格下載 URL（如果存在），或在 README 中說明本地副本位置

**問題 2：Section 14 Report ID Optimization 描述**
- 文件提到 "Report ID >= 15: Low byte = 0x0F (sentinel)"，但未提供完整的 sentinel 編碼範例
- 建議補充具體的 3-byte 格式範例，以便實作者參考

---

### 6. HID-over-I2C/02-Architecture/ARCHITECTURE.md ✅

**路徑：** `doc/HID-over-I2C/02-Architecture/ARCHITECTURE.md`
**大小：** 11KB（目錄清單顯示 11,047B）

- **技術準確性：** ✅ USB HID Device + I²C Host 雙重角色、M487 韌體分層架構、EP0/EP1/EP2 配置、I²C 位址 0x2E、Memory Layout、中斷優先權設定皆正確
- **來源確認：** ✅ 基於 SPEC.md 的架構推導，無外部不可靠來源
- **完整性：** ✅ 涵蓋 System Overview、Bridge Architecture、Layer Structure、USB Interface、Translation Layer、HID Descriptor Caching、Device Discovery、Error Handling、Memory Layout、Interrupt Priorities、Design Decisions
- **連結有效：** ✅ 無外部連結
- **格式一致性：** ✅ 表格、架構圖 ASCII 一致
- **錯字/語法：** ✅ 無

---

### 7. HID-over-I2C/03-Protocol/TRANSLATION.md ✅ → ⚠️

**路徑：** `doc/HID-over-I2C/03-Protocol/TRANSLATION.md`
**大小：** 17.6KB（目錄清單顯示 17,598B）

#### ✅ 通過項目
- **來源確認：** ✅ 基於 SPEC.md，無不可靠來源
- **完整性：** ✅ 涵蓋所有 USB HID → I²C 操作映射、Interrupt Flow、Register Access Protocol、HID Descriptor Retrieval、Report Format Translation、Command Reference、Timing Requirements、Error Recovery
- **格式一致性：** ✅ 時序圖、表格、命令編碼表一致
- **錯字/語法：** ✅ 無

#### ⚠️ 需修正

**問題 1：SET_IDLE 命令格式範例與 Opcode Table 不一致（重要）**
- **位置：** Section 1.6 SET_IDLE
  ```
  Write Command Register:
    [0x02][0x00|Duration]
  ```
- **說明：** Section 6.2 Opcode Table 正確定義了 SET_IDLE opcode = 0x05（即 `byte 0 = (0x05 << 4) = 0x50`）。但 Section 1.6 的範例錯誤地使用了 `0x02`（reserved opcode = 0）
  - 正確格式應為：`[0x50][0x00|Duration]` 或 `[(0x05 << 4)][Duration]`（若 low nibble 無 reserved bits，則 `[0x50][rate]`）
  - 與 SPEC.md §5 的 opcode encoding 完全一致（opcode 在 byte 0 的 high nibble）
- **嚴重性：** Major（與 Opcode Table 自相矛盾，誤導實作者）
- **建議修正：**
  ```
  Write Command Register:
    [0x50][Duration]  # opcode=0x05, high nibble=0x5
  ```

**問題 2：I²C 設備角色描述語義模糊**
- **位置：** Section 2
  ```
  I²C side: M487 acts as I²C host controller. The peripheral device is a HID DEVICE that implements HID over I²C protocol.
  ```
- **說明：** 「The peripheral device is a HID DEVICE」的描述容易與 USB HID Device 混淆（因為 M487 本身就是 USB HID Device）。建議改為：
  ```
  The peripheral device is an I²C HID Device that implements the HID-over-I²C protocol.
  ```
- **嚴重性：** Minor（語義問題，不影響技術正確性）

**問題 3：MLX90614 身份說明不足**
- 文件提及 MLX90614 作為「similar to HID-over-I²C」的範例，但未明確說明 MLX90614 **並非** HID-over-I²C 設備（它是 SMBus 設備）。見 EXAMPLE.md 的同一問題。

---

### 8. HID-over-I2C/04-Registers/REGISTERS.md ✅

**路徑：** `doc/HID-over-I2C/04-Registers/REGISTERS.md`
**大小：** 7.9KB（目錄清單顯示 7,878B）

- **技術準確性：** ✅ USB EP 配置（EPA=64B, EPB=64B）、Buffer 位址、I²C Register Indices、Data Structures（C 結構體）、Command Opcodes 與 SPEC.md 一致、USB Report IDs 配置合理、Memory Map 與 ARCHITECTURE.md 一致
- **來源確認：** ✅ 基於 SPEC.md 和 ARCHITECTURE.md，無外部不可靠來源
- **完整性：** ✅ 涵蓋所有 Register definitions、Data Structures、Command Opcodes、USB Report IDs、Memory Map
- **連結有效：** ✅ 無外部連結
- **格式一致性：** ✅ C 程式碼區塊、指標表格一致
- **錯字/語法：** ✅ 無

---

### 9. HID-over-I2C/05-Example/EXAMPLE.md ✅ → ⚠️

**路徑：** `doc/HID-over-I2C/05-Example/EXAMPLE.md`
**大小：** 4.8KB（目錄清單顯示 4,833B）

#### ✅ 通過項目
- **I²C 通訊序列：** Read/Write/Data Register 流程、HID Descriptor 讀取時序皆正確
- **格式一致性：** ✅ C 程式碼區塊、資料表一致
- **錯字/語法：** ✅ 無

#### ⚠️ 需修正

**問題 1：MLX90614 非 HID-over-I²C 設備 — 說明不足（重要）**
- **位置：** 文件標題與開頭說明
- **說明：** MLX90614 是 **SMBus** 設備，不是 HID-over-I²C 設備。文件建立了一個「虛擬 HID-over-I²C 包裝層」來模擬 HID-over-I²C 行為。這一點在文件中說明不足，讀者可能誤以為 MLX90614 原生支援 HID-over-I²C
- **建議：** 在文件開頭增加明確聲明：
  > ⚠️ **注意：** MLX90614 本身是 SMBus 設備，不支援 HID-over-I²C 協議。本範例建立了一個虛擬 HID-over-I²C 包裝層來展示 bridge 與設備的互動模式，而非實際的 HID-over-I²C 設備範例。如需驗證，建議使用真正支援 HID-over-I²C 的設備（如特定觸控板或感測器）

**問題 2：HID Descriptor 中 wMaxInputLength 數值**
- HID Descriptor 中宣告 `wMaxInputLength = 4` (0x04)，但實際 Temperature Data 為 4 bytes，加上 2-byte length prefix 後，長度字段應為 0x06 (6)。宣告 0x04 會讓 host 以為最多只能讀 4 bytes，造成資料截斷
- **建議：** 將 `wMaxInputLength` 改為 `0x06, 0x00`（6 bytes，包含 length field 本身）

**問題 3：I2C_ReadRegister(addr, ...)：位址格式未說明**
- 文件使用 `addr = 0x5A`（7-bit），`I2C_ReadRegister` API 是否需要自行處理 R/W bit 附加未說明
- 建議加註說明 API 內部是否自動處理 7-bit vs 8-bit 位址

---

### 10. HID-over-I2C/06-TestPlan/TEST_PLAN.md ✅

**路徑：** `doc/HID-over-I2C/06-TestPlan/TEST_PLAN.md`
**大小：** 7.0KB（目錄清單顯示 6,959B）

- **技術準確性：** ✅ Unit/Integration/System/Compatibility 分層測試模型、測試 ID 命名規範、Pass Criteria、Test Environment 描述皆合理且完整
- **來源確認：** ✅ 純內部測試策略文件
- **完整性：** ✅ 涵蓋 4 個測試層級、Test Deliverables、Schedule
- **連結有效：** ✅ 無外部連結
- **格式一致性：** ✅ 測試矩陣、表格一致
- **錯字/語法：** ✅ 無

---

### 11. HID-over-I2C/07-Review/REVIEW.md ✅

**路徑：** `doc/HID-over-I2C/07-Review/REVIEW.md`
**大小：** 6.6KB（目錄清單顯示 6,617B）

- **技術準確性：** ✅ Code/Integration/Final Review 分級、檢查清單（規格一致性/架構一致性/實作品質/測試覆蓋/文件完整性）、Review 流程、Pass Criteria 皆正確且完整
- **來源確認：** ✅ 純內部流程文件
- **完整性：** ✅ 涵蓋 Review 等級定義、檢查清單、報告格式、流程、Pass Criteria
- **連結有效：** ✅ 無外部連結
- **格式一致性：** ✅ 表格一致
- **錯字/語法：** ✅ 無

---

### 12. Task_Template.md ✅

**路徑：** `tasks/Task_Template.md`
**大小：** 489B

- **技術準確性：** ✅ 任務模板結構完整（基本資訊、目標、需求、進度、備註）
- **完整性：** ✅ 必要欄位齊備
- **連結有效：** ✅ 無外部連結
- **格式一致性：** ✅ 表格格式一致
- **錯字/語法：** ✅ 無

---

### 13. Verify_Template.md ✅

**路徑：** `tasks/Verify_Template.md`
**大小：** 412B

- **技術準確性：** ✅ 驗收模板結構完整（基本資訊、交付清單、驗證結果、Blocks、備註）
- **完整性：** ✅ 必要欄位齊備
- **連結有效：** ✅ 無外部連結
- **格式一致性：** ✅ 表格格式一致
- **錯字/語法：** ✅ 無

---

## 🔍 發現的問題彙整

### ❗ Major（需優先修正）

| ID | 文件 | 問題 | 影響 |
|----|------|------|------|
| M-01 | TRANSLATION.md §1.6 | SET_IDLE 命令格式範例 `[0x02][...]` 與 Opcode Table (`opcode=0x05`) 自相矛盾，正確應為 `[0x50][...]` | 誤導實作者，與 SPEC.md 不一致 |
| M-02 | EXAMPLE.md | MLX90614 非 HID-over-I²C 設備，文件未明確說明包裝層模擬本質 | 讀者可能誤用 MLX90614 作為 HID-over-I²C 設備參考 |
| M-03 | EXAMPLE.md | HID Descriptor `wMaxInputLength=4`，但實際溫度資料 4 bytes + length prefix = 6 bytes，會導致截斷 | 實作時 host 無法完整接收資料 |

### ⚠️ Minor（建議修正）

| ID | 文件 | 問題 | 影響 |
|----|------|------|------|
| m-01 | VSCode_IDE_Setup.md | Quick Start 連結路徑 `../ICE/QUICK_START.md` 語法不夠明確 | 路徑可能因解析環境而失效 |
| m-02 | M487_Usage_Guide.md | 外部 URL（Nuvoton 官網下載連結）無法驗證，建議備份或以 GitHub BSP 為主要來源 | 連結可能失效 |
| m-03 | NuLink_Compilation.md | 內部連結路徑 `../ICE/` 應為 `../../ICE/` | 路徑解析冗餘但結果正確 |
| m-04 | SPEC.md | 本地來源檔案路徑無法驗證，建議加附 Microsoft 官方連結 | 無法確認來源完整性 |
| m-05 | TRANSLATION.md §2 | 「peripheral device is a HID DEVICE」語義模糊，與 M487 的 USB HID Device 身份混淆 | 理解上的困惑 |

---

## 💡 建議

### 立即修正（Next Sprint）

1. **修正 TRANSLATION.md §1.6**：將 `[0x02][0x00|Duration]` 改為 `[0x50][Duration]`（依據 Opcode Table opcode=0x05）
2. **修正 EXAMPLE.md wMaxInputLength**：從 `0x04, 0x00` 改為 `0x06, 0x00`
3. **在 EXAMPLE.md 開頭增加聲明**：明確說明 MLX90614 為 SMBus 設備，本範例使用虛擬包裝層

### 建議改進（的文件更新週期）

4. **統一內部相對路徑語法**：所有從子目錄到同級子目錄的連結，統一使用 `../../` 而非 `../`
5. **將外部 URL 改為 GitHub 鏡像或本地備份**：Nuvoton 官網下載連結可靠性低，建議以 GitHub BSP 作為主要參考
6. **在 SPEC.md 加入官方連結**：Microsoft HID over I²C Protocol Specification v1.0 的官方下載 URL

### 長期建議

7. **建立文件版本追蹤機制**：所有文件加 `last_verified` 欄位，每次實作變更時同步更新文件版本號

---

## 📈 整體評估

**通過標準：** ✅ 無 ❌ 項目，Major 問題需修正後方可宣告文件體系完整

**文件體系質量評估：**
- **HID-over-I2C 文件群：** 技術內容嚴謹，覆蓋完整，但有 3 個 Major 問題需修正
- **IDE/NuLink 文件群：** 實務操作性強，但內部連結語法需統一
- **Template 文件群：** 結構清晰，可直接使用

**總體評估：⚠️ CONDITIONAL PASS**
> 發現 3 個 Major 問題需要修正後，才能將此文件體系作為可靠的工程參考文件。修正優先順序：TRANSLATION.md > EXAMPLE.md

---

*Dororo — 驗證完畢*
*2026-04-07*
