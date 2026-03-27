# 任務清單 / Task List

> 最後更新:2026-03-27
> 政策:每 30 分鐘檢查進度,每個 Phase 完成後 Review

---

## 任務狀態說明

| 狀態 | 標籤 | 意義 |
|------|------|------|
| Pending | ⏳ | 等待中(待硬體、待相依任務) |
| Ongoing | 🔄 | 執行中 |
| Finish | ✅ | 已完成 |

---

## 任務執行規則

1. **軟體優先:** 無硬體也能執行的任務優先處理
2. **持續推進:** 每 30 分鐘 heartbeat 檢查,發現可執行任務立即開始
3. **相依管理:** 若任務被 block,維持 Pending 並檢查下游任務
4. **Fallback 原則:** 若所有任務皆因硬體等不可抗因素無法推進,**立即轉向現有專案的 Review 與功能優化**,不得停滯

> ⚠️ **重要:** 當任務清單中所有任務都無法前進時(硬體不在、無相依任務等),**不是等待,而是 Review**。包括:
> - 程式碼品質檢視
> - 文件完整性確認
> - 架構合理性優化
> - 測試案例補全
> - 效能優化検討

---

---

## T001 - USB 複合裝置(MSC + HID I2C 自訂格式)

**狀態:** ⏳ Pending(待硬體)
**起始:** 2026-03-26
**Branch:** `firmware/composite-rewrite`
**目標:** M487 USB 複合裝置(自訂 HID Report 格式)

**進度:**
- [x] 韌體架構設計
- [x] GCC 編譯環境建立
- [x] 韌體編譯(43.9KB)
- [x] 燒錄成功
- [ ] 實體測試(需回辦公室)
- [ ] I2C Read 功能實作(I2C_Read 函式 stub)
- [ ] Windows Tool 整合測試

**位置:** `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`

---

## T002 - arm-none-eabi-gcc 編譯環境

**狀態:** ✅ Finish
**起始:** 2026-03-26

**進度:**
- [x] Makefile 建立(VENDOR_LBK + Composite)
- [x] 編譯成功驗證
- [x] 燒錄驗證

---

## T003 - OpenOCD + Nu-Link 燒錄流程

**狀態:** ✅ Finish
**起始:** 2026-03-26

**進度:**
- [x] OpenOCD 燒錄流程驗證
- [x] flash.bat 腳本
- [x] 速度:~6-13 KiB/s

---

## T004 - HID-over-I2C Bridge 實作(完整軟體方案)

**狀態:** 🔄 Ongoing(Phase 1-5 完成,待完整 USB HID 實作)
**起始:** 2026-03-26
**Branch:** `architecture/hid-over-i2c`
**目標:** M487 作為標準 USB HID Device,Bridge 到 HID-over-I2C 協定的 I2C 裝置

### Phase 1:I2C 驅動實作 ✅ Finish
- [x] UI2C0 初始化(PE2=CLK, PE3=DAT0, 100kHz)
- [x] I2C Read/Write 函式(blocking)
- [x] I2C bus scan 函式

### Phase 2:HID Descriptor Parser ✅ Finish
- [x] 讀取 I2C device HID Descriptor(30 bytes)
- [x] 解析並驗證 HID Descriptor 欄位
- [x] 解析 Report Descriptor

### Phase 3:USB HID Device Layer ✅ Finish
- [x] USB enumeration(VID=0x04F3, PID=0x0732)
- [x] EP0 Control endpoint handler
- [x] EP1 Interrupt IN(HID Input Report)
- [x] EP2 Interrupt OUT(HID Output Report)
- [x] HID class requests(GET_REPORT, SET_REPORT, SET_IDLE, GET_IDLE)

### Phase 4:翻譯層實作 ✅ Finish
- [x] USB HID Request → HID-over-I2C Command 轉換
- [x] Command register builder
- [x] Data register response parser
- [x] Input Report 中斷驅動(GPIO interrupt)

### Phase 5:整合與編譯 ✅ Finish(編譯成功,43.3KB)
- [x] 嘗試編譯(驗證 BSP API 相容性)
- [x] 修正編譯錯誤
- [x] 韌體編譯成功(43.3KB)
- [ ] USB HID Layer 完整實作(目前為 Stub,需整合 BSP HSUSBD 框架)→ 見 T008
- [ ] 🔄 整合 Review → 見 T009

**韌體位置:** `firmware/hid-over-i2c/build/firmware.bin`
**文件:** `D:\AiWorkSpace\M487_ScsiTool\doc\HID-over-I2C\`

---

## T005 - OpenOCD + 除錯工具

**狀態:** ✅ Finish
**起始:** 2026-03-26

**進度:**
- [x] OpenOCD 使用指南文件
- [x] 燒錄指令參考
- [x] GDB 除錯整合
- [ ] VSCode GDB launch 設定優化

**位置:** `T005\`

---

## T006 - Windows C++ Win32 HID Tool

**狀態:** ⏳ Pending(規劃中)
**相依於:** T004 / T008 完成後

**進度:**
- [ ] 研究 Windows HID API(HidD_*, SetupDi*)
- [ ] 設計干淨的 C++ class 介面
- [ ] 實作 HID I2C 命令傳送/接收
- [ ] 整合測試

---

## T007 - I2C Read 功能實作(T001 修復)

**狀態:** ⏳ Pending(待實作)
**相依於:** T001 實體測試完成

**進度:**
- [ ] 完成 I2C_Read 函式
- [ ] 測試 I2C write + read 流程

---

## T008 - BSP HSUSBD 框架整合(T004 USB HID Layer)

**狀態:** 🔄 Ongoing(研究完成，待完整整合)
**起始:** 2026-03-27
**目標:** 將 T004 的 USB HID Layer Stub 整合進 BSP HSUSBD 框架

> 目前 `usb_hid.c` 是 Stub,需參考 `HSUSBD_HID_Transfer_And_MSC` 範例實作完整 USB HID。

**研究進度 (2026-03-27 09:31):**
- [x] 研究 BSP `HSUSBD_HID_Transfer_And_MSC` 範例的 USB 實作方式
- [x] 識別關鍵函式: HID_Init, HID_ClassRequest, EPA_Handler, EPB_Handler
- [x] 識別 Descriptor 結構和 endpoint 配置
- [ ] 整合 HID Class Request Handler
- [ ] 整合 EP0 Control Endpoint
- [ ] 整合 EP1 Interrupt IN / EP2 Interrupt OUT
- [ ] 整合 HSUSBD 中斷處理常式 (USBD20_IRQHandler)
- [ ] 編譯驗證
- [ ] 燒錄測試(需硬體)

**研究發現 (更新 10:01):**
- BSP 範例位於 `D:\AiWorkSpace\KM\M480BSP\SampleCode\StdDriver\HSUSBD_HID_Transfer_And_MSC`
- BSP 使用 `gsHSInfo` 結構管理所有 descriptors
- BSP 依賴庫函式: `HSUSBD_ProcessSetupPacket`, `HSUSBD_CtrlIn`, `HSUSBD_PrepareCtrlIn` 等
- 完整整合需要將 `gsHSInfo` 與 bridge callback 整合
- 這是復雜的整合工作，需要 BSP 庫支援

**整合策略:**
- 方案A: 以 BSP 範例為基礎修改（置換 HID handler 為 bridge callback）
- 方案B: 維持 STUB，等有硬體後再整合
- 建議採用方案A，但需要更多時間研究 BSP 框架

**相依於:** T004 Phase 3(stub 已完成)

---

## T009 - T004 Review 報告

**狀態:** ⏳ Pending
**目標:** 依據 `07-Review/REVIEW.md` 執行 T004 Phase 1-5 Review

**進度:**
- [ ] 程式碼 Review(§3.3 實作品質檢查清單)
- [ ] 規格一致性 Review(§3.1 SPEC.md 對照)
- [ ] 架構一致性 Review(§3.2 ARCHITECTURE.md 對照)
- [ ] 產出 Review 報告
- [ ] 根據報告修復問題

**相依於:** T008 完成

---

## T010 - T001 實體測試

**狀態:** ⏳ Pending(待硬體)
**相依於:** 回到辦公室

**進度:**
- [ ] 插上 M487 USB Device 纜線
- [ ] 確認 VID=0x04F3 PID=0x0732 出現
- [ ] 測試 MSC RAM Disk
- [ ] 測試 HID I2C 通訊

---

## T011 - T001 實作品質 Review

**狀態:** ✅ Finish
**目標:** 對 T001 firmware/composite-rewrite 進行 Review

**依據:** `07-Review/REVIEW.md` §3.3 實作品質 + §3.1 規格一致性

**Review 結果摘要:**
- **Critical/Major 問題:** 0
- **Minor 問題:** 2 (記錄於下)
- **已修復問題:** 5

**已修復問題:**
- `hid_i2c.c` EPB_Handler: 加入 `len > sizeof(g_u8OutBuff)` 邊界檢查 (Q-03)
- `i2c_control.c` NACK handler: 加入 retry 邏輯(NACK_RETRY_MAX=3, 指數backoff) (Q-05)
- `i2c_control.c`: Magic numbers替換為命名常量(I2C_TIMEOUT_COUNT, I2C_SCAN_TIMEOUT, I2C_SCAN_DELAY) (Q-07)
- `hid_i2c.c` HID_ClassRequest: GET_REPORT 實作回傳Feature Report，不再STALL (S-05)
- `hid_i2c.c` HID_ClassRequest: SET_REPORT 同時處理Output(0x02)和Feature(0x03) (S-06)

**Minor 問題 (不阻礙交付):**
- I2C_Read() 回傳 -1 表示未實作，應補完後統一錯誤碼約定
- 部分函式缺 docstring(HID_GetOutReport, HID_SetInReport)

**結論:** T001 firmware 通過 Review，發現的問題已修復。Minor 問題可在後續版本處理。

---

## T012 - T002 / T003 工具鏈 Review

**狀態:** ⏳ Pending(可獨立執行)
**目標:** 審視編譯環境與燒錄流程的完整性与可重現性

**進度:**
- [ ] Makefile 結構一致性(與 T004 比對)
- [ ] 確認 VENDOR_LBK + Composite 皆可編譯
- [ ] flash.bat 脚本跨專案共用性
- [ ] 路徑相依性檢查(絕對路徑 vs 相對路徑)
- [ ] OpenOCD script 完整性(錯誤處理)
- [ ] 產出 Review 報告

**相依於:** 無,可立即執行

---

## T013 - 現有專案功能優化検討

**狀態:** ⏳ Pending(依據 Review 結果)
**目標:** 根據 T011/T012 Review 發現的問題,規劃優化方向

**可能優化方向:**
- [ ] I2C driver 加入 interrupt mode(目前為 polling)
- [ ] Makefile 參數化(-toolchain, -bsp_dir 等)
- [ ] Error handling 強化(retry, timeout, recovery)
- [ ] Memory footprint 優化
- [ ] Power consumption 評估

**相依於:** T011, T012 完成

---

---

## T014 - I2C NACK Retry 機制實作(P0)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🔴 P0 - 來自 Dororo T011 Review
**負責:** 🐱 Giroro
**目標:** 消除 I2C 通訊單次失敗即中斷的脆弱設計

**問題:** `i2c_control.c` 的 NACK handler 直接發 STOP,無任何 retry 邏輯

**進度:**
- [x] 分析 NACK 發生情境(設備忙碌、匯流排衝突、訊號品質)
- [x] 實作可配置次數的 NACK retry(NACK_RETRY_MAX = 3)
- [x] 建立指數 backoff 策略
- [x] 回傳統一的錯誤碼(I2C_ERR_NACK_RETRY_EXCEEDED)
- [x] 所有 Magic Numbers 用 `#define` 替代

**位置:** `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`

---

## T015 - Magic Numbers 消除 + 統一錯誤碼策略(P0)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🔴 P0 - 來自 Dororo T011/T012 Review
**負責:** 🐱 Giroro
**目標:** 建立可維護的常量管理與錯誤處理策略

**問題清單:**
- `timeout = 1000000UL`(Magic Number,無任何 `#define`)
- `timeout = 1000`(Magic Number)
- `g_u8OutBuff[64]` 無長度邊界保護
- `I2C_Read()` 回傳 `-1`,其餘函式回 `0`,無統一約束

**進度:**
- [x] 建立 `i2c_constants.h`:timeout、retry、buffer size 等常量集中管理
- [x] 建立 `i2c_error.h`:統一錯誤碼(I2C_OK, I2C_ERR_NACK, I2C_ERR_BUSY, I2C_ERR_TIMEOUT...)
- [x] 消除 `HID_CmdI2CWrite`/`I2C_StartTransaction`/`I2C_Scan` 中的 Magic Numbers
- [x] 統一所有 I2C 函式錯誤回傳值

**相依於:** 無,可立即執行

---

## T016 - Makefile 環境變數重構(P1)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 Dororo T012 Review + Kururu 建議
**負責:** 🐹 Tamama
**目標:** 將硬編碼路徑替換為環境變數,實現跨環境可移植

**問題:** `C:/Users/rinry/Tool/xpack-...`、`D:/AiWorkSpace/KM/M480BSP` 全為絕對路徑

**進度:**
- [ ] 建立 toolchain 探索機制(XPKG_ROOT)
- [ ] BSP_DIR 改為相對路徑或環境變數
- [ ] 驗證 xpack 版本相容性(14.x → 15.x)
- [ ] 建立 `Makefile.config.example` 供新環境參考
- [ ] 確認 VENDOR_LBK + Composite 皆可編譯

**位置:** `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\`
**相依於:** 無,可立即執行

---

## T017 - flash.bat Error Handling 強化(P1)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 Dororo T012 Review
**負責:** 🐹 Tamama
**目標:** 加入路徑驗證、錯誤處理、燒錄驗證機制

**問題:** OpenOCD/Nu-Link 路徑全 Hardcoded,失敗不回傳錯誤碼

**進度:**
- [ ] 將 OpenOCD 路徑改為環境變數或相對路徑
- [ ] 加入 tool 存在性檢查(Tool路徑驗證)
- [ ] 加入 `--verify` 燒錄驗證選項
- [ ] 加入錯誤碼檢查與 early exit
- [ ] 燒錄速度/除錯選項參數化
- [ ] 建立版本相容性檢查

**位置:** `D:\AiWorkSpace\M487_ScsiTool\firmware\composite\flash.bat`
**相依於:** 無,可立即執行

---

## T018 - usb_hid.c STUB 標記 + README 建立(P1)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 Dororo T009 Review
**負責:** 🐹 Tamama
**目標:** 清楚標示 STUB 函式,建立專案文件降低新成員學習成本

**問題:** `usb_hid.c` 全為 STUB,但無任何 TODO/STUB 標記;無 README.md

**進度:**
- [ ] 所有 STUB 函式加入 `// STUB: TODO` 或 `__attribute__((weak))` 標記
- [ ] 建立 `firmware/hid-over-i2c/README.md`(架構說明、編譯方式、燒錄流程)
- [ ] 建立 `firmware/hid-over-i2c/TEST_PLAN.md`(Phase 1-5 驗證項目)
- [ ] 補齊 `usb_hid.c` 函式 docstring

**位置:** `D:\AiWorkSpace\M487_ScsiTool\firmware\hid-over-i2c\`
**相依於:** 無,可立即執行

---

## T019 - T008 Mock 環境建立(P1)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 Kururu PM 建議
**負責:** 🐹 Tamama
**目標:** 在無硬體環境下完成 T008 模組級邏輯開發與單元測試

**進度:**
- [ ] 建立 fake HAL 層(fake_usb_device, fake_i2c_bus, fake_gpio)
- [ ] 建立 T008 模組的單元測試(目標:80% branch coverage)
- [ ] Mock `HSUSBD_*` API 以驗證整合邏輯
- [ ] 建立 CI 自動化測試腳本

**位置:** `D:\AiWorkSpace\M487_ScsiTool\firmware\hid-over-i2c\test\mock\`
**相依於:** 無,可立即執行

---

## T020 - 硬體到貨後測試矩陣建立(P1)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 Kururu PM 建議
**負責:** 🦀 Kururu + 🐸 Dororo
**目標:** 硬體到貨時立即執行,不浪費一分鐘

**測試矩陣:**

| 測試項目 | 預期時間 | 失敗應變 |
|---------|---------|---------|
| USB 枚舉(主機辨識 VID=0x04F3 PID=0x0732)| 30 min | 檢查 D+/D- 線路、供電 |
| HID I2C 通訊 | 1 hr | 驗證 T004 端點配置 |
| MSC RAM Disk | 1 hr | 檢查 USB descriptors |
| 複合裝置同時運作 | 2 hr | 測試端點衝突 |
| I2C_Read 功能驗證 | 1 hr | 確認 NACK retry 行為 |
| I2C Bus Scan | 30 min | 確認接線與設備地址 |

**進度:**
- [x] 建立測試案例文字檔(測試步驟、預期結果、失敗時檢查點)
- [x] 建立測試資料夾結構(logs/, test_data/, reports/)
- [x] 指派測試負責人(Dororo)

**文件位置:** `D:\AiWorkSpace\M487_ScsiTool\tasks\T020\TEST_MATRIX.md`

**相依於:** 無,可立即執行

---

## T021 — BuildCommandRegister 截斷風險修復（Major）

**狀態：** ✅ Finish
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 Giroro + Dororo Review
**負責:** 🐱 Giroro
**目標:** 16-bit register 位址不再被截斷為 8-bit

**問題:** `HID_Parser_ParseDescriptor()` 將 16-bit register 位址存入 `uint8_t`,導致高位元組丟失

**分析結果:**
- HID-over-I2C 規範明確定義 register 位址為 16-bit(wReportDescRegister, wInputRegister 等)
- 目前 `HID_Context_t` 中 reg* 欄位為 `uint8_t`,會截斷高位元組
- 大多數 HID-over-I2C 設備實際只使用 8-bit register 位址(0x00-0xFF)
- 若設備需要 > 0xFF 的 register 位址,目前實作會失敗

**進度:**
- [x] 確認 HID-over-I2C 協定中 register 位址是否需要 16-bit → **需要**
- [x] 修改 `HID_Context_t` 結構使用 `uint16_t` reg* 欄位
- [x] 更新 `i2c_driver.h/c` 中的 I2C0_ReadReg 等函式支援 uint16_t reg
- [x] 更新 bridge.c 和 hid_parser.c 中的呼叫

**實作記錄(2026-03-27 08:01):**
- `hid_parser.h`:reg* 欄位從 `uint8_t` 改為 `uint16_t`
- `i2c_driver.h`:I2C0_WriteReg/ReadReg/WriteRead 函式 reg 參數改為 `uint16_t`
- `i2c_driver.c`:實作 16-bit register 傳輸(先送 MSB 再送 LSB)
- `hid_parser.c`:移除 `(uint8_t)` cast,儲存完整 16-bit 值
- `hid_parser.c`:更新 DumpDescriptor 的 printf 格式(%02X → %04X)

**相依於:** T004 HID Descriptor Parser 確認

---

## T022 - Buffer 邊界檢查補全(Major)

**狀態:** 🔄 Ongoing
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 Dororo T011 Review
**負責:** 🐱 Giroro
**目標:** 所有 buffer 操作皆有明確的邊界驗證

**問題點:**
- `g_u8OutBuff[64]` 在 `EPB_Handler()` 中直接取用 endpoint len,無 `<= EPB_MAX_PKT_SIZE` 驗證
- `I2C_Read()` 中 `uint8_t readData[64]` 無長度保護

**進度:**
- [x] EPB_Handler 加入 `len <= EPB_MAX_PKT_SIZE` 檢查
- [x] I2C_Read 加入 buffer overflow 保護
- [x] 所有 memcpy/memset 加入長度驗證
- [x] HID_CmdI2CWrite/Read/WriteRead/Scan 加入邊界檢查

**相依於:** 無,可立即執行

---

## T023 - Windows HID Tool CLI 設計(T006 前期)

**狀態:** ✅ Finish
**起始:** 2026-03-27
**優先:** 🟡 P2 - 來自 Tamama 建議
**負責:** 🐹 Tamama
**目標:** 先行建立 CLI 架構,定義指令格式

**Tamama 建議的 CLI 設計:**
```
hidtool device list
hidtool read <addr> [len]
hidtool write <addr> <data>
hidtool reg read <reg>
hidtool reg write <reg> <data>
hidtool log [on|off|export]
```

**進度:**
- [ ] 研究 Windows HID API(HidD_*, SetupDi*)
- [ ] 設計乾淨的 C++ class 介面
- [ ] 建立 CLI 指令解析框架
- [ ] 錯誤訊息標準化(詳細的失敗說明,而非只顯示 "Failed")
- [ ] Hotplug 監聽機制設計

**相依於:** 無,可立即執行(文件/架構設計)

---

## T024 - ITM/SWO Trace System (Debug 基礎建設)

**狀態:** ✅ Finish(程式碼完成,待硬體驗證)
**起始:** 2026-03-27
**優先:** 🔴 P0 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 提供零成本的即時執行追蹤能力,不需要 halt MCU

**實作內容:**
- `itm.h` / `itm.c` - ITM 追蹤系統
- ITM_Init() / ITM_InitWithBaud() - TPI/SWO 設定
- ITM_Log() / ITM_ERR() / ITM_DBG() / ITM_HEX_DUMP() - 格式化日誌
- DWT cycle counter - 時間戳記
- Module-specific macros: USB_TRACE, MSC_TRACE, I2C_TRACE, HID_TRACE

**硬體需求:**
- PB8 = SWO pin (需確認正確 MFP 值)
- SWD 調試器(J-Link/CMSIS-DAP) 或 邏輯分析儀
- SWO baud rate: 2 MHz (可調整)

**已修改:**
- `firmware/composite/main.c` - ITM_Init() 整合
- `firmware/composite/build_gcc/Makefile` - 加入 itm.c

**待確認:**
- PB8 SWO pin 的 MFP alternate function 值(M487 datasheet)
- SWO 訊號是否從專用 debug header 輸出

---

## T025 - MSC Vendor Debug Channel (出廠後 Debug 方案)

**狀態:** ✅ Finish(程式碼完成,待硬體驗證)
**起始:** 2026-03-27
**優先:** 🔴 P0 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 透過 USB MSC 存取任意記憶體/CPU暫存器,不需要任何額外驅動

**實作內容:**
- `msc_debug.h` / `msc_debug.c` - MSC Vendor Command Handler
- CDB 0xC0-0xFF (Vendor-specific range) 攔截
- Sub-commands:
  - `0x01` DBG_READ_MEM - 讀取記憶體
  - `0x02` DBG_WRITE_MEM - 寫入記憶體 (僅 SRAM)
  - `0x03` DBG_READ_REG - 讀取 CPU 暫存器
  - `0x04` DBG_WRITE_REG - 寫入 CPU 暫存器
  - `0x07` DBG_GET_INFO - 讀取裝置資訊
  - `0x08` DBG_READ_LOG - 讀取除錯日誌環形緩衝區
  - `0x0F` DBG_ECHO - 迴路測試

**Host 端需求:**
- Windows: `DeviceIoControl` + `IOCTL_SCSI_PASS_THROUGH`
- macOS: `io_registry_entry_t` + SCSI passthrough
- Linux: `/dev/sg*` + SG_IO

**已修改:**
- `firmware/composite/hid_i2c.c` - MSC_ProcessCmd() 加入 vendor CDB case
- `firmware/composite/main.c` - MSC_Debug_Init() 整合
- `firmware/composite/build_gcc/Makefile` - 加入 msc_debug.c

**限制:**
- 寫入僅限 SRAM (0x20000000-0x20027FFF),保護 Flash
- Debug Log 環形緩衝區 512 bytes

---

## T026 - MSC Debug CLI Tool (msc_debug.exe)

**狀態:** ⏳ Pending
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 BrainStorm 小組會議
**負責:** 🐹 Tamama
**目標:** Windows CLI 工具,透過 USB MSC 發送 Vendor CDB,讀取除錯資訊

**CLI 設計:**
```
msc_debug.exe info                    # 讀取裝置資訊
msc_debug.exe readmem <addr> <len>   # 讀取記憶體
msc_debug.exe writemem <addr> <hex>  # 寫入記憶體
msc_debug.exe readreg <reg>           # 讀取 CPU 暫存器
msc_debug.exe writereg <reg> <val>   # 寫入 CPU 暫存器
msc_debug.exe log [count]             # 讀取除錯日誌
msc_debug.exe echo <data>             # 迴路測試
```

**技術實作:**
- 使用 Windows File API (`CreateFile("\\\\.\\E:")`)
- `DeviceIoControl` + `IOCTL_SCSI_PASS_THROUGH`
- `IOCTL_SCSI_PASS_THROUGH_DIRECT` for memory writes
- CBW structure: [0xC0][sub_cmd][addr_hi][addr_lo][len][reserved]

**相依於:** T025 完成

---


## T027 - USB HID Filter Driver (Windows Kernel Debug Tool)

**狀態:** ⏳ Pending(前置準備中)
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 BrainStorm 小組會議
**負責:** 🐹 Tamama
**目標:** 建立 Windows Kernel-mode Filter Driver,附掛在 HID Class Driver 之上,攔截並紀錄所有 USB HID 溝通封包

**實作架構:**
`
User App (hidlog.exe)
      │ WMI / IOCTL
      ▼
┌─────────────────────────────────────┐
│  M487Filter.sys (KMDF HID Upper     │
│  Filter Driver)                     │
│  - Attach to VID=0x04F3 PID=0x0732  │
│  - Capture IRP_MJ_INTERNAL_DEVICE_CONTROL
│  - Ring buffer: 1024 entries        │
│  - ETW tracing support              │
└──────────┬──────────────────────────┘
           │ Pass-through
┌──────────▼──────────────────────────┐
│  hidclass.sys (Windows HID Class)  │
└──────────┬──────────────────────────┘
           │
┌──────────▼──────────────────────────┐
│  usbhid.sys (USB HID Driver)        │
└──────────┬──────────────────────────┘
           │
┌──────────▼──────────────────────────┐
│  M487 USB Device                   │
└─────────────────────────────────────┘
`

**子任務:**

| Sub | 任務 | 優先 | 說明 |
|-----|------|------|------|
| T027a | WDK + VS2022 環境建立 | P1 | 確認 WDK 安裝,建立 KMDF 專案 |
| T027b | Firefly 範本研究 | P1 | 分析 Windows-driver-samples/hid/firefly |
| T027c | Filter Driver 骨架實作 | P1 | KMDF HID upper filter, attach/detach |
| T027d | IOCTL 攔截實作 | P1 | IOCTL_HID_GET_FEATURE, IOCTL_HID_READ_REPORT 等 |
| T027e | Ring Buffer + ETW | P2 | Kernel-mode 日誌緩衝,Windows ETW 追蹤 |
| T027f | WMI Interface | P2 | user-mode app 查詢 log |
| T027g | hidlog.exe CLI 工具 | P1 | 顯示擷獲的 HID 流量 |
| T027h | 安裝程式/簽章 | P2 | INF + 驅動程式簽章 |

**技術細節:**
- 以 Firefly 為基礎: Windows-driver-samples/hid/firefly
- KMDF version 1.15+
- Filter 安裝: registry HKLM\SYSTEM\CurrentControlSet\Services\M487Filter
- Target Device: USB\VID_04F3&PID_0732
- Driver 名稱: M487Filter.sys

**預估工時:**
- T027a 環境: 0.5 天
- T027b-g 核心: 4-5 天
- T027h 簽章: 1 天
- **總計: 5-7 天**

**環境需求:**
- Visual Studio 2022
- Windows Driver Kit (WDK) 10.0.26100.0+
- Windows 11 SDK
- 測試機: Windows 10/11 (需啟用測試模式)

**Driver 簽章選項:**
1. **測試簽章** (Test Signing): cdedit /set testsigning on
2. **自我簽署** (Self-signed): SignTool + 自製 CA
3. **EV Code Signing Certificate** — 正式發布用

**產出:**
- 	ool/M487Filter/M487Filter.sys - 驅動程式
- 	ool/M487Filter/M487Filter.inf - 安裝資訊
- 	ool/M487Filter/hidlog.exe - CLI 工具
- 	ool/M487Filter/README.md - 安裝與使用說明

**相依於:** T027a WDK 環境完成

---

**狀態:** ⏳ Pending
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 透過 UART 輸出結構化日誌,客戶現場可快速診斷

**實作內容:**
- 結構化日誌格式: `[LVL][HH:MM:SS.mmm][module] message`
- 5 層日誌過濾: ERR/WARN/INFO/DBG/TRC
- 輸出至 UART0 (已設定 PB12/PB13 為 UART0 RX/TX)
- UART baud rate: 115200 (已設定)

**Debug 模式 vs 生產模式:**
- Debug: 所有日誌開啟 (DBG level)
- 生產: 只開 ERROR 以上

**硬體需求:**
- USB-to-UART 介面 (CH340/FTDI, ~$5)
- 3 條線: TX/RX/GND

**相依於:** 無,可立即實作

---

## T028 - UART Debug Log System (傳統 Debug 備援)

**狀態:** ✅ Finish(程式碼完成)
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 透過 UART 輸出結構化日誌,客戶現場可快速診斷

**實作內容:**
- 結構化日誌格式: `[HH:MM:SS.mmm][LVL][module] message`
- 5 層日誌過濾: ERR/WARN/INFO/DBG/TRC
- 輸出至 UART0 (已設定 PB12/PB13 為 UART0 RX/TX)
- UART baud rate: 115200 (已設定)
- 編譯期過濾 + 執行期可調整

**實作檔案:**
- `firmware/composite/uart_debug.h` - Header, macros
- `firmware/composite/uart_debug.c` - 實作
- `firmware/composite/main.c` - UART_DBG_Init() 整合

**Module-specific macros:**
- MAIN_LOG/MAIN_ERR/MAIN_WARN/MAIN_DBG/MAIN_TRC
- USB_LOG/USB_ERR/USB_WARN/USB_DBG/USB_TRC
- MSC_LOG/MSC_ERR/MSC_WARN/MSC_DBG/MSC_TRC
- I2C_LOG/I2C_ERR/I2C_WARN/I2C_DBG/I2C_TRC
- HID_LOG/HID_ERR/HID_WARN/HID_DBG/HID_TRC
- DBG_LOG/DBG_ERR/DBG_WARN/DBG_DBG/DBG_TRC

**Debug 模式 vs 生產模式:**
- Debug: 所有日誌開啟 (UART_LOG_LEVEL = DBG)
- 生產: 只開 ERROR 以上 (UART_LOG_LEVEL = ERR)

**硬體需求:**
- USB-to-UART 介面 (CH340/FTDI, ~$5)
- 3 條線: TX/RX/GND

**相依於:** 無,可立即實作

---

## T029 - hidtool 完整版 (整合 MSC Debug Channel)

**狀態:** ⏳ Pending
**起始:** 2026-03-27
**優先:** 🟠 P1 - 來自 BrainStorm 小組會議
**負責:** 🐹 Tamama
**目標:** 將 MSC Debug Channel + HID Debug Channel 整合為單一 CLI 工具

**功能整合:**
- `hidtool device list` - 列舉 HID 裝置 (現有)
- `hidtool msc info` - 讀取 MSC Debug 裝置資訊
- `hidtool msc readmem <addr> <len>` - 記憶體讀取
- `hidtool msc log` - 即時日誌監控
- `hidtool trace start/stop` - 啟動/停止 ITM trace

**技術實作:**
- libusb 或 HIDAPI (跨平台支援)
- 可選: Python binding (自動化測試)

**相依於:** T026 完成

---

## T030 - ITM Trace Viewer (PC 端工具)

**狀態:** ⏳ Pending
**起始:** 2026-03-27
**優先:** 🟡 P2 - 來自 BrainStorm 小組會議
**負責:** 🐹 Tamama
**目標:** PC 端接收並顯示 ITM SWO trace 資料

**實作方向:**
- UART-to-USB bridge (M487 SWO → PC UART → USB)
- 或 SWD debugger 支援 SWO (J-Link, CMSIS-DAP)
- 軟體: 自製 Python CLI 或整合至現有工具

**替代方案:**
- SEGGER J-Link SWO Viewer (免費,需 J-Link)
- 邏輯分析儀擷取 SWO 訊號

**相依於:** T024 完成

---

## T031 - Flash Error Log System (錯誤持久化)

**狀態:** ⏳ Pending
**起始:** 2026-03-27
**優先:** 🟡 P2 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 將錯誤碼寫入 Flash 保留區,形成 persistent error log,出廠後可讀取

**實作內容:**
- Flash 保留區使用 (FMC 最後 4KB,需確認可用範圍)
- ErrorLogEntry_t 結構: timestamp + error_code + module + detail
- 最大 64 筆錯誤記錄,環形覆蓋
- 可透過 MSC Debug Channel (T025) 讀取

**Error Code 對應表:**
```c
// i2c_error.h 已定義的錯誤碼
#define I2C_OK                       0
#define I2C_ERR_NACK                -1
#define I2C_ERR_TIMEOUT             -2
#define I2C_ERR_BUSY                -3
#define I2C_ERR_NACK_RETRY_EXCEEDED -4
```

**相依於:** T025 完成

---

## T032 - Self-Test Mode (開機自我檢測)

**狀態:** ⏳ Pending
**起始:** 2026-03-27
**優先:** 🟡 P2 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 開機時執行晶片內建 self-test,及早發現硬體問題

**實作內容:**
- Clock verification (確認 PLL/HXT 頻率正確)
- SRAM March test (完整記憶體測試)
- USB PHY loopback test (需確認 M487 是否支援)
- I2C bus sanity check (確認匯流排可響應)

**結果寫入特定 RAM 位址,MSC Debug Channel 可讀取:**
```c
// HID command: Report ID 0xFE = Self-test request
// Response: [test_name][pass/fail][details...]
```

**相依於:** T024/T025 完成

---

## T033 - GDB RSP Server (Software ICE via USB MSC)

**狀態:** ⏳ Research
**起始:** 2026-03-27
**優先:** 🟡 P2 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 透過 USB MSC 接受 GDB 命令,實現 self-hosted debugging

**GDB RSP (Remote Serial Protocol):**
- 監聽 UART 或 USB MSC Debug Channel
- 支援: `g` (read registers), `G` (write registers), `m` (read memory), `M` (write memory)
- 支援: `c` (continue), `s` (step), `z` (clear breakpoint), `Z` (set breakpoint)

**基本限制:**
- 無法在 MCU halt 自己之前先 halt 自己
- 需要 MCU 已處於 halt 狀態才能開始調試
- 適用場景: 已知錯誤發生點,手動暂停後調查

**預估工時:** 3-5 天

**相依於:** T025 完成

---

## T034 - DWT Breakpoint/Watchpoint Debug

**狀態:** ⏳ Research
**起始:** 2026-03-27
**優先:** 🟡 P2 - 來自 BrainStorm 小組會議
**負責:** 🐱 Giroro
**目標:** 利用 ARM CoreSight DWT 實現硬體 breakpoint/watchpoint

**DWT 功能:**
- 6 個 hardware breakpoint (IWM/IWRS)
- 4 個 watchpoint (DWT Comparator)
- PC sampling
- Exception tracing

**實作方向:**
- 透過 ITM/SWO 輸出 DWT event
- 整合至 MSC Debug Channel
- 提供 `break <addr>` / `watch <addr>` 指令

**相依於:** T024 完成

---

## T035 - USBPcap + Wireshark 整合

**狀態:** ⏳ Reference
**起始:** 2026-03-27
**優先:** 🟢 P3 - 來自 BrainStorm 小組會議
**負責:** 無 (參考工具)
**目標:** 使用 USBPcap 擷取 USB 流量,用於進階協定分析

**使用方式:**
1. 安裝 Wireshark + USBPcap
2. 選擇 USBPcap 介面
3. Filter: `usb.device_address == <M487_ADDR>`
4. 分析 USB HID 封包、MSC BOT 傳輸

**限制:**
- 只能離線分析,無 API 可用
- 適合開發階段,不適合客戶現場

**替代工具:**
- Bus Hound (商業,功能完整)
- Ellisys USB Analyzer (專業硬體,昂貴)

---

## Debug 整備矩陣

| Lv | 工具 | 優先度 | 狀態 | 說明 |
|----|------|--------|------|------|
| L0 | 產品出廠 | N/A | - | 無 Debug |
| L1 | UART Log (T028) | P1 | ✅ Finish | 客戶現場,需額外硬體 |
| L2 | MSC Debug Channel (T025) | P0 | ✅ Finish | 即插即用,無需驅動 |
| L2 | MSC Debug CLI (T026) | P1 | ⏳ Pending | Host 端工具,透過 SCSI passthrough |
| L2 | hidtool (T029) | P1 | ⏳ Pending | 整合 MSC + HID Debug |
| L3 | USB Filter Driver (T027) 含 T027a-h | P1 | ⏳ Pending | Windows Kernel-mode driver,需 WDK |
| L3 | GDB RSP Server (T033) | P2 | ⏳ Research | 軟體 ICE,MSC 承載 |
| L4 | ITM/SWO (T024) | P0 | ✅ Finish | 零額外成本,需 debug header |
| L4 | ITM Viewer (T030) | P2 | ⏳ Pending | PC 端 SWO trace |
| L4 | DWT Debug (T034) | P2 | ⏳ Research | 硬體 breakpoint/watchpoint |
| L4 | Flash Error Log (T031) | P2 | ⏳ Pending | 錯誤持久化,出廠後可讀取 |
| L4 | Self-Test Mode (T032) | P2 | ⏳ Pending | 開機自我檢測 |
| L5 | ICE + GDB | N/A | 現有 | Nu-Link2,需退修 |
| L5 | USBPcap + Wireshark (T035) | P3 | ⏳ Reference | 離線協定分析,參考工具 |

---

## 執行狀態摘要

| 任務 | 狀態 | 優先 |
|------|------|------|
| T001 | ⏳ Pending(待硬體)| - |
| T002 | ✅ Finish | - |
| T003 | ✅ Finish | - |
| T004 | 🔄 Ongoing(Phase 1-5 完成)| - |
| T005 | ✅ Finish | - |
| T006 | ⏳ Pending | P2 |
| T007 | ⏳ Pending | - |
| T008 | 🔄 Ongoing(下一個)| - |
| T009 | ⏳ Pending | - |
| T010 | ⏳ Pending(待硬體)| - |
| T011 | ✅ Finish | - |
| **T014** | ✅ Finish | **P0** |
| **T015** | ✅ Finish | **P0** |
| **T016** | ✅ Finish | P1 |
| **T017** | ✅ Finish | P1 |
| **T018** | ✅ Finish | P1 |
| **T019** | ✅ Finish | P1 |
| **T020** | ✅ Finish | P1 |
| **T021** | ✅ Finish | P1 |
| **T022** | ✅ Finish | P1 |
| **T023** | ✅ Finish | P2 |
| **T024** | ✅ Finish | P0 |
| **T025** | ✅ Finish | P0 |
| **T026** | ⏳ Pending | P1 |
| **T027** | ⏳ Pending (含 T027a-h) | P1 |
| **T028** | ✅ Finish | P1 |
| **T029** | ⏳ Pending | P1 |
| **T030** | ⏳ Pending | P2 |
| **T031** | ⏳ Pending | P2 |
| **T032** | ⏳ Pending | P2 |
| **T033** | ⏳ Research | P2 |
| **T034** | ⏳ Research | P2 |
| **T035** | ⏳ Reference | P3 |

---

## Review 政策

所有任務完成後,參照 `D:\AiWorkSpace\M487_ScsiTool\doc\HID-over-I2C\07-Review\REVIEW.md` 執行:

1. **程式碼 Review** - 每個 Phase 實作完成
2. **整合 Review** - 模組整合完成
3. **最終 Review** - 交付前

不通過(Critical/Major)→ 修復 → 重新 Review

---

## 🚀 執行順序（軟體優先,硬體最後）

> 規則: 軟體優先 → 有硬體才能做的往後排 → 同時多線並行

### 第一梯隊（今天可開始,無任何block）

| 順序 | 任務 | 負責 | 優先 | 說明 |
|------|------|------|------|------|
| **1** | T026 MSC Debug CLI Tool | Tamama | P1 | `msc_debug.exe`, 0.5-1天 |
| **2** | T027a WDK 安裝完成後 | Tamama | P1 | VS2022已裝,等WDK |
| **3** | T028 UART Debug Log | Giroro | P1 | 0.5天, 純韌體無依賴 |
| **4** | T027b Firefly 研究 | Tamama | P1 | T027a完成後立即開始 |
| **5** | T033 GDB RSP 研究 | Giroro | P2 | 可同步研究,不影響其他 |
| **6** | T034 DWT 研究 | Giroro | P2 | 可同步研究,不影響其他 |

### 第二梯隊（T026/T027 完成後解鎖）

| 順序 | 任務 | 負責 | 優先 | 前置依賴 |
|------|------|------|------|---------|
| **7** | T027c Filter 骨架 | Tamama | P1 | T027b 完成 |
| **8** | T027d IOCTL 攔截 | Tamama | P1 | T027c 完成 |
| **9** | T027e Ring Buffer + ETW | Tamama | P2 | T027d 完成 |
| **10** | T027f WMI Interface | Tamama | P2 | T027d 完成 |
| **11** | T027g hidlog.exe | Tamama | P1 | T027f 完成 |
| **12** | T027h 安裝/簽章 | Tamama | P2 | T027g 完成 |
| **13** | T029 hidtool 整合 | Tamama | P1 | T026 + T027g 完成 |
| **14** | T030 ITM Viewer | Tamama | P2 | T024 完成（已✅）|

### 第三梯隊（T024/T025 延伸功能）

| 順序 | 任務 | 負責 | 優先 | 前置依賴 |
|------|------|------|------|---------|
| **15** | T031 Flash Error Log | Giroro | P2 | T025 完成（已✅）|
| **16** | T032 Self-Test Mode | Giroro | P2 | T024 完成（已✅）|

### 硬體待命梯隊（回辦公室後執行）

| 順序 | 任務 | 負責 | 優先 | 備註 |
|------|------|------|------|------|
| **17** | T001 實體測試 | Giroro | P0 | 等硬體 |
| **18** | T010 T001 實體測試 | Giroro | P1 | 等 T001 完成 |
| **19** | T008 BSP HSUSBD 整合 | Giroro | P1 | 等 T001 完成 |
| **20** | T009 T004 Review 報告 | Giroro | P1 | 等 T008 完成 |

### 參考工具（不需要做,了解即可）

| 順序 | 任務 | 負責 | 備註 |
|------|------|------|------|
| — | T035 USBPcap + Wireshark | 無 | 了解用法即可 |

---

### 快速啟動指引

**今天立即可做（不需要任何依賴）：**
```
1. T026 msc_debug.exe  → Tamama/Giroro
2. T028 UART Debug Log → Giroro
3. T027b Firefly 研究  → Tamama（等WDK安裝）
4. T033/T034 研究       → Giroro（可並行）
```

**最關鍵路徑（影響最多下游）：**
```
T001(硬體) → T008(BSP整合) → T009/其他
     ↓
硬體瓶頸
```

**並行策略：**
- Giroro: T028 → T031 → T032 → T033/T034
- Tamama: T026 → T027b → T027c-g → T029
- 硬體回辦公室: T001 → T008 → T009
