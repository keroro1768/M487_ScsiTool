# Windows Forms 自定義控制項

## 來源
- Microsoft Learn: https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls-design/overview

---

## 三種自定義控制項類型

### 1. 衍生現有控制項 (Extended Control)
- 繼承現有的控制項
- 新增額外功能
- 最簡單的方式

### 2. 複合控制項 (User Control)
- 繼承 `UserControl`
- 包含多個現有控制項
- 可重用的 UI 元件

### 3. 完全自訂控制項 (Custom Control)
- 繼承 `Control`
- 完全自己繪製
- 最高彈性

---

## 建立 UserControl (複合控制項)

### 步驟

1. 在 Visual Studio 中新增 `UserControl` 項目
2. 設計 UI（拖放控制項）
3. 加入自訂屬性和事件

### 範例：登入控制項

```csharp
public partial class LoginControl : UserControl
{
    // 自訂屬性
    public string Title 
    { 
        get => lblTitle.Text; 
        set => lblTitle.Text = value; 
    }
    
    public event EventHandler<LoginEventArgs> Login;
    
    private void btnLogin_Click(object sender, EventArgs e)
    {
        // 驗證並引發事件
        if (ValidateInputs())
        {
            Login?.Invoke(this, new LoginEventArgs(Username, Password));
        }
    }
}

// 自訂事件參數
public class LoginEventArgs : EventArgs
{
    public string Username { get; }
    public string Password { get; }
    
    public LoginEventArgs(string user, string pass)
    {
        Username = user;
        Password = pass;
    }
}
```

---

## 建立 Custom Control (完全自訂)

### 基本結構

```csharp
public class MyCustomControl : Control
{
    protected override void OnPaint(PaintEventArgs e)
    {
        base.OnPaint(e);
        
        // 自訂繪製
        using (var brush = new SolidBrush(Color.FromArgb(0, 120, 215)))
        {
            e.Graphics.FillRectangle(brush, 0, 0, Width, Height);
        }
        
        // 繪製文字
        TextRenderer.DrawText(e.Graphics, Text, Font, 
            new Point(10, 10), Color.White);
    }
}
```

### 加入屬性

```csharp
// 顏色屬性
private Color _fillColor = Color.DodgerBlue;
public Color FillColor
{
    get => _fillColor;
    set 
    { 
        _fillColor = value; 
        Invalidate(); // 重新繪製
    }
}

// 圖示屬性
private Image _icon;
public Image Icon
{
    get => _icon;
    set { _icon = value; Invalidate(); }
}
```

### 加入事件

```csharp
public event EventHandler Clicked;

// 引發自訂事件
protected void OnClicked()
{
    Clicked?.Invoke(this, EventArgs.Empty);
}
```

---

## 設計階段支援

### 加入設計工具屬性

```csharp
[Category("Appearance")]
[Description("The fill color of the control")]
[DefaultValue(typeof(Color), "DodgerBlue")]
public Color FillColor { get; set; }
```

### 加入設計工具編輯器

```csharp
[Editor(typeof(System.Windows.Forms.Design.FileNameEditor), 
        typeof(UITypeEditor))]
public string FilePath { get; set; }
```

---

## Owner-Drawn 自繪製控制項

### ListBox 自繪製範例

```csharp
public class CustomListBox : ListBox
{
    public CustomListBox()
    {
        DrawMode = DrawMode.OwnerDrawFixed;
        DrawItem += CustomListBox_DrawItem;
    }
    
    private void CustomListBox_DrawItem(object sender, DrawItemEventArgs e)
    {
        e.DrawBackground();
        
        // 自訂繪製
        var item = Items[e.Index];
        
        using (var brush = new SolidBrush(e.ForeColor))
        {
            e.Graphics.DrawString(item.ToString(), Font, brush, e.Bounds);
        }
        
        e.DrawFocusRectangle();
    }
}
```

---

## 屬性分類

| 類別 | 說明 |
|------|------|
| Appearance | 外觀相關 |
| Behavior | 行為相關 |
| Layout | 佈局相關 |
| Data | 資料相關 |
| Category | 自訂分類 |

---

## 官方資源

| 資源 | 連結 |
|------|------|
| UserControl 教學 | https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls-design/how-to-create-usercontrol |
| Custom Control 概述 | https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls-design/overview |
| 簡單自訂控制項 | https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls-design/how-to-create-simple-custom-control |
| Owner-Drawn | https://learn.microsoft.com/en-us/dotnet/desktop/winforms/controls-design/custom-controls-overview |

---

*建立時間: 2026-03-25*
