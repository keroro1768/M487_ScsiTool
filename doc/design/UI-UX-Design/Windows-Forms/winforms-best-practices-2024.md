# Windows Forms Best Practices 2024

## 來源
- MESCIUS: https://developer.mescius.com/blogs/winforms-development-best-practices-for-2024

## 核心概念

WinForms 應用程式開發最佳實踐，透過設計模式和最佳實踐實現簡潔透明的開發流程。

## 架構模式

### MVC (Model-View-Controller)

| 元件 | 職責 |
|------|------|
| **Model** | 資料管理、業務邏輯 |
| **View** | UI 呈現、使用者介面 |
| **Controller** | 處理輸入、協調 Model 和 View |

### MVP (Model-View-Presenter)

| 元件 | 職責 |
|------|------|
| **Model** | 資料和業務邏輯 |
| **View** | UI 呈現（被動） |
| **Presenter** | 商業邏輯，控制 View 顯示 |

## 關鍵原則

### 1. 關注點分離 (Separation of Concerns)

```
錯誤做法：
Form1.cs
├── btn_Click() {
│   ├── 讀取資料庫  ← 業務邏輯
│   ├── 處理資料    ← 業務邏輯  
│   ├── 顯示在 UI  ← UI 邏輯
│   └── 寫入檔案    ← 業務邏輯
│   }
}
```

```
正確做法：
├── Models/
│   └── DataService.cs     # 業務邏輯
├── Views/
│   └── Form1.cs          # 只負責 UI
└── Controllers/
    └── Form1Controller.cs # 協調
```

### 2. UI 邏輯與業務邏輯分開

- 讓 Form/UserControl 專注於 UI
- 業務邏輯放在獨立的類別
- 提高可測試性和可維護性

### 3. 使用 UserControls

將 UI 拆分為邏輯上分組的小控制項：
- 更易於重新設計 UI 佈局
- 提高代碼重用性
- 更容易單元測試

## 控制項設計

### 標籤與文字方塊對齊

| 建議 | 說明 |
|------|------|
| 標籤寬度一致 | 建議 100-150px |
| 文字方塊寬度一致 | 根據內容調整 |
| 間距統一 | 5-10px |

### 選擇正確的控制項

| 場景 | 建議控制項 |
|------|-----------|
| 單一選擇 | RadioButton / ComboBox |
| 多重選擇 | CheckBox / CheckedListBox |
| 大量選項 | ComboBox / ListView |
| 布林值 | CheckBox / ToggleSwitch |

## 參考資源

- [MESCIUS WinForms Best Practices](https://developer.mescius.com/blogs/winforms-development-best-practices-for-2024)
- [Stack Overflow WinForms UI Guide](https://stackoverflow.com/questions/4692186/best-practice-ui-guide)
- [Mark Heath - Maintainable WinForms](https://markheath.net/post/maintainable-winforms)

---

*建立時間: 2026-03-25*
