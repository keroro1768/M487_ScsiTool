# Windows Forms 完整佈局指南

## 來源
- Microsoft Learn: https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls/layout

## 1. 基本定位 (Fixed Position and Size)

### Location 屬性
- 控制項位置由 `Location` 屬性決定
- 座標 `(x, y)` 相對於父容器的左上角

### Size 屬性
- 控制項大小由 `Size` 屬性決定
- 包含 `Width` 和 `Height`

### 最大/最小尺寸
```csharp
textBox1.MaximumSize = new Size(300, 100);
textBox1.MinimumSize = new Size(100, 20);
```

---

## 2. Margin vs Padding

### Margin (外邊距)
- 控制項**外部**與其他控制項的距離
- 維持控制項邊界與其他控制項的空間

### Padding (內邊距)
- 控制項**內部**內容與邊界的距離
- 維持內容（如文字）與控制項邊界的空間

### 視覺示意
```
┌─────────────────────────────────────┐
│            Padding (內邊距)           │
│  ┌───────────────────────────────┐   │
│  │      Margin (外邊距)        │   │
│  │  ┌───────────────────────┐   │   │
│  │  │     控制項內容        │   │   │
│  │  └───────────────────────┘   │   │
│  └───────────────────────────────┘   │
└─────────────────────────────────────┘
```

### 程式碼設定
```csharp
button1.Margin = new Padding(10);
button1.Padding = new Padding(5);
```

---

## 3. Dock (停靠)

### 概念
- 將控制項「停靠」到容器的某一边緣
- 當容器大小改變時，控制項會自動調整大小

### Dock 模式
| 模式 | 說明 |
|------|------|
| Top | 停靠上方，寬度自動調整 |
| Bottom | 停靠下方，寬度自動調整 |
| Left | 停靠左側，高度自動調整 |
| Right | 停靠右側，高度自動調整 |
| Fill | 填滿整個容器 |
| None | 不停靠 |

### 範例
```csharp
//經典的 Header - Content - Footer 佈局
headerPanel.Dock = DockStyle.Top;      // 顶部
contentPanel.Dock = DockStyle.Fill;   // 中間填滿
footerPanel.Dock = DockStyle.Bottom;   // 底部
```

### Z-Order 影響
- 先停靠的控制項會先佔用空間
- 調整 Z-Order 可以改變停靠順序

---

## 4. Anchor (錨定)

### 概念
- 將控制項「錨定」到容器的某一边或多邊
- 當容器大小改變時，控制項維持與錨定邊的距離

### 錨定組合
```csharp
// 預設：左上角錨定
button1.Anchor = AnchorStyles.Top | AnchorStyles.Left;

// 四角錨定（維持相對位置）
button1.Anchor = AnchorStyles.Top | AnchorStyles.Left | 
                  AnchorStyles.Bottom | AnchorStyles.Right;

// 右下角錨定
button1.Anchor = AnchorStyles.Right | AnchorStyles.Bottom;
```

### 視覺示例
```
調整前              調整後
┌──────────┐       ┌──────────────┐
│ Button   │       │      Button  │  ← Right 錨定
│     □    │   →   │         □   │
└──────────┘       └──────────────┘
```

---

## 5. FlowLayoutPanel (流態佈局)

### 特點
- 自動由左至右、由上至下排列控制項
- 適合動態產生控制項清單

### 屬性
```csharp
flowLayoutPanel1.FlowDirection = FlowDirection.TopDown;  // 流向
flowLayoutPanel1.WrapContents = true;                   // 自動換行
flowLayoutPanel1.AutoScroll = true;                    // 自動捲動
```

---

## 6. TableLayoutPanel (表格佈局)

### 特點
- 像 HTML Table 一樣的網格佈局
- 適合對齊的控制項矩陣

### 設定
```csharp
tableLayoutPanel1.ColumnCount = 3;
tableLayoutPanel1.RowCount = 4;

// 設定欄寬比例
tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));
tableLayoutPanel1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 50F));

// 設定列高比例
tableLayoutPanel1.RowStyles.Add(new RowStyle(SizeType.Absolute, 30F));
```

---

## 7. SplitContainer (分割容器)

### 特點
- 將容器分為兩個可調整大小的區域
- 適合 Sidebar + Content 佈局

### 範例
```csharp
splitContainer1.Orientation = Orientation.Vertical;  // 垂直分割
splitContainer1.SplitterDistance = 200;               // 初始位置
splitContainer1.IsSplitterFixed = false;              // 可調整
```

---

## 8. Panel vs GroupBox vs UserControl

| 容器 | 特性 | 使用場景 |
|------|------|---------|
| **Panel** | 簡單容器，無邊框 | 分組相關控制項 |
| **GroupBox** | 有邊框和標題 | 功能群組 |
| **UserControl** | 可自訂複合控制項 | 重用的 UI 元件 |

---

## 9. 實用佈局範例

### 經典表單佈局
```csharp
// 使用 TableLayoutPanel
tableLayoutPanel1.ColumnCount = 2;
tableLayoutPanel1.RowCount = 5;

// 標籤欄位
tableLayoutPanel1.Controls.Add(new Label { Text = "姓名：" }, 0, 0);
tableLayoutPanel1.Controls.Add(textBoxName, 1, 0);

// 按鈕列
tableLayoutPanel1.SetColumnSpan(btnSubmit, 2);
```

### 回應式佈局
```csharp
private void Form1_Resize(object sender, EventArgs e)
{
    // 保持按鈕在右下角
    btnSave.Left = this.Width - btnSave.Width - 20;
    btnSave.Top = this.Height - btnSave.Height - 20;
}
```

---

## 10. 設計原則

### 對齊
- 使用格線對齊
- 相同類型的控制項寬度一致

### 間距
- 統一的 Margin 和 Padding
- 相關控制項靠近

### 層次
- 重要資訊放在上方/左方
- 使用 GroupBox 分組

---

## 官方資源

| 資源 | 連結 |
|------|------|
| 微軟官方文檔 | https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls/layout |
| 控制項手冊 | https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls/ |
| 設計工具 | Visual Studio Designer |

---

*建立時間: 2026-03-25*
