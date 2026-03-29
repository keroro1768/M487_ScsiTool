# Tasks — 任務管理

> 所有任務的集中管理區

## 📁 資料夾結構

```
tasks/
├── Readme.md           ← 本說明文件
├── TaskList.md         ← 任務總清單
├── Task_Template.md    ← Task.md 任務模板
├── Verify_Template.md ← VERIFY.md 驗收模板
│
├── T001/
│   ├── Task.md         ← 任務說明（立項即存在）
│   ├── PLAN.md         ← 執行計畫（規劃時建立）
│   ├── WORKLOG.md      ← 工作日誌（立項即存在）
│   └── VERIFY.md       ← 驗收結果（done 後才新增）
│
├── T002/
│   └── ...
│
└── T027/               ← 母任務
    ├── Task.md
    ├── PLAN.md
    ├── WORKLOG.md
    ├── VERIFY.md
    ├── T027a/         ← 子任務實體化
    ├── T027b/
    └── T027c~
```

## 📄 Task.md（任務說明）

立項時建立，包含目標、需求、進度。

## 📄 PLAN.md（執行計畫）

規劃階段建立，包含具體執行步驟、子任務拆分、預估工時。

## 📄 WORKLOG.md（工作日誌）

立項即存在，每次工作後更新。

## 📄 VERIFY.md（驗收結果）

**done 後才建立**，包含交付清單、驗證結果、 Blocks。

## 🔢 Task 編號規則

| 範圍 | 內容 |
|------|------|
| T001-T005 | 基礎建設 |
| T006-T013 | 規劃中功能 |
| T014-T023 | P0/P1 優先修復 |
| T024-T025 | Debug 系統 |
| T026 | MSC Debug CLI Tool |
| T027 | USB Filter Driver（含 T027a-h 子任務）|
| T028-T035 | 延伸功能 |

## 📋 任務狀態

| 標記 | 意義 |
|------|------|
| ⏳ 待處理 | 等待執行 |
| 🔄 進行中 | 正在執行 |
| ✅ 完成 | 已完成 |
| ⏸️ 暫停 | 硬體 Pending |
| ⏰ 延後 | 暫時擱置 |
