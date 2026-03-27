# Desktop Application Layout Design

## 來源
- JustInMind: https://www.justinmind.com/ui-design/layout-website-mobile-apps
- UX StackExchange: https://ux.stackexchange.com/questions/3414/desktop-software-design-patterns

## 桌面 vs 網頁 vs 手機

| 特性 | 桌面應用 | 網頁應用 | 手機應用 |
|------|---------|---------|---------|
| 螢幕空間 | 最大 | 中等 | 最小 |
| 輸入設備 | 鍵盤+滑鼠 | 鍵盤+滑鼠 | 觸控 |
| 使用時間 | 較長 | 中等 | 短暫 |
| 功能複雜度 | 高 | 中等 | 低 |

## 桌面應用的獨特設計模式

### 1. Ribbon Interface (功能區)

常見於 Microsoft Office 系列應用

```
┌─────────────────────────────────────────────┐
│ [檔案] [編輯] [檢視] ...                    │
├─────────────────────────────────────────────┤
│ [常用] [插入] [版面配置] [郵件]              │
│ ─────────────────────────────────────────── │
│  ■ 粗體  │ 斜體 │ 底線 │ 字體 │ 大小     │
└─────────────────────────────────────────────┘
```

**優點：**
- 減少點擊次數
- 功能一目了然
- 適合功能複雜的應用

### 2. Dock Panels (停靠面板)

```
┌─────────────────────────────────┬──────┐
│                                 │ 屬性 │
│                                 │ 面板  │
│        主要工作區               ├──────┤
│                                 │ 圖層 │
│                                 │ 面板  │
├─────────────────────────────────┼──────┤
│ [檔案總管]                      │ 工具 │
│                                 │ 面板  │
└─────────────────────────────────┴──────┘
```

**優點：**
- 可自訂佈局
- 節省空間
- 快速存取常用功能

### 3. Master-Detail (主從視圖)

```
┌──────────────────┬──────────────────────────┐
│                  │                          │
│    列表區        │       詳細內容區          │
│                  │                          │
│  - 項目 1      │  項目 1 的完整內容...    │
│  - 項目 2      │                          │
│  - 項目 3      │                          │
│                  │                          │
└──────────────────┴──────────────────────────┘
```

**適用場景：**
- 電子郵件客戶端
- 檔案總管
- 客戶管理系統

### 4. Modal vs Modeless Dialogs

| 類型 | 行為 | 適用場景 |
|------|------|---------|
| **Modal** | 阻塞主視窗 | 重要確認、設定 |
| **Modeless** | 可同時操作 | 搜尋結果、工具視窗 |

### 5. Contextual UI (情境 UI)

根據使用者上下文顯示不同的 UI：

- 右鍵選單
- 浮動工具列
- 智慧標籤 (Smart Tags)

## 設計原則

### 1. 資訊層次 (Information Hierarchy)

```
┌─────────────────────────────────────────┐
│ ★ 標題 (最大，最醒目)                    │
│ ─────────────────────────────────────── │
│ 主要內容 (清晰易讀)                       │
│                                         │
│   次要資訊 (較小，灰色)                   │
└─────────────────────────────────────────┘
```

### 2. 導航效率

- 快捷鍵支援
- 常用功能快速存取
- 清晰的麵包屑導航

### 3. 視覺化

- 圖示輔助識別
- 顏色編碼
- 拖放操作支援

### 4. 狀態回饋

- 處理中的指示
- 進度條
- 錯誤/成功訊息

## Windows 設計指南

Microsoft 官方設計指南：

- [Windows App Design Guidelines](https://learn.microsoft.com/en-us/windows/apps/design/)
- Windows 11 設計原則
- Fluent Design System

---

## 參考資源

- [JustInMind - UI Layout Design](https://www.justinmind.com/ui-design/layout-website-mobile-apps)
- [UX StackExchange - Desktop Design Patterns](https://ux.stackexchange.com/questions/3414/desktop-software-design-patterns)
- [Microsoft - Windows App Design](https://learn.microsoft.com/en-us/windows/apps/design/)

---

*建立時間: 2026-03-25*
