# T041 - hid_bridge / tool/hidtool_cpp 測試驗證

**狀態:** ⏳ 待處理  
**建立:** 2026-04-13  
**類型:** Testing  
**負責:** 🦀 Dororo  
**依賴:** T040（需文件確認後執行）  
**花費:** TBD  

---

## 概述

2026-04-13 新增 C++ HID 工具代碼，需測試覆蓋驗證。

### 目標目錄

- `D:\AiWorkSpace\M487_ScsiTool\hid_bridge\` (含編譯產物 `build/M487HidTool.exe`)
- `D:\AiWorkSpace\M487_ScsiTool\tool\hidtool_cpp\`

---

## 測試項目

### hid_bridge

- [ ] CMake 建置是否成功（無警告）
- [ ] `build/M487HidTool.exe` 是否存在且可執行
- [ ] `hid_device` 跨平台 API 單元測試（如有測試框架）
- [ ] `hid_i2c_bridge` 邏輯測試

### tool/hidtool_cpp

- [ ] CMake 建置是否成功
- [ ] 與 Python `hidtool.py` 功能重疊評估
- [ ] 是否需要整合至現有 `tool/hidtool/` 流程

### 測試案

| ID | 測試項目 | 預期結果 | 實際結果 |
|----|---------|---------|---------|
| T041-01 | hid_bridge CMake build | 成功 | - |
| T041-02 | M487HidTool.exe --help | 顯示幫助 | - |
| T041-03 | tool/hidtool_cpp CMake build | 成功 | - |

---

## 驗收標準

- [ ] 建置測試完成（CMake 成功）
- [ ] 功能測試記錄完整
- [ ] 與現有工具重疊性評估完成
