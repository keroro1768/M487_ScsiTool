# WinDbg 指令大全與斷點對應

## 來源
- Microsoft Learn
- Stack Overflow

---

## 一、斷點指令 (Breakpoint Commands)

### 1. 基本斷點語法

| 指令 | 說明 |
|------|------|
| `bp` | 在地址設定斷點 |
| `bm` | 在符號/模式上設定斷點 |
| `bu` | 在未解析的符號上設定斷點 |
| `ba` | 在記憶體存取上設定斷點 |

### 2. 根據原始碼行號設定斷點

```powershell
# 方法1：使用 source:line 語法
bp /f /l:10 main.cpp

# 方法2：使用 bpmd (適用於 .NET)
bpmd myapp.exe MyNamespace.MyClass.Method

# 方法3：使用絕對地址
bp 0x00401000
```

### 3. 斷點指令參數

| 參數 | 說明 |
|------|------|
| `/f` | 一次觸發後自動刪除 |
| `/1` | 一次觸發後停用 |
| `/t` | 執行緒 ID |
| `/c` | 呼叫堆疊深度 |
| `/l` | 原始碼行號 |
| `/p` | 程式碼頁 |

---

## 二、Source Mode 設定

### 開啟原始碼模式

```powershell
# 設定原始碼路徑
.srcpath+ C:\myproject\src

# 開啟原始碼視窗
lm m mymodule

# 設定符號路徑
.sympath+ C:\myproject\ symbols
```

### 顯示原始碼

```powershell
# 列出原始碼 (需要 PDB)
.list

# 顯示目前位置周圍的原始碼
x /s main!*
```

---

## 三、常用 WinDbg 指令

### 執行控制

| 指令 | 說明 |
|------|------|
| `g` 或 `go` | 繼續執行 |
| `p` | 逐步執行 (不進入函數) |
| `t` | 逐步執行 (進入函數) |
| `pa` | 逐步執行到指定位址 |
| `pc` | 逐步執行到下一個函數呼叫 |
| `gu` | 執行到函數結束 |
| `wt` | 追蹤並顯示統計 |

### 記憶體與暫存器

| 指令 | 說明 |
|------|------|
| `r` | 顯示暫存器 |
| `d*` | 顯示記憶體 (db, dw, dd, dq, da) |
| `e*` | 編輯記憶體 |
| `k` | 顯示呼叫堆疊 |
| `kv` | 詳細呼叫堆疊 |

### 模組與符號

| 指令 | 說明 |
|------|------|
| `lm` | 列出載入的模組 |
| `x` | 列出符號 |
| `ln` | 列出附近的符號 |
| `.reload` | 重新載入符號 |

### 執行緒

| 指令 | 說明 |
|------|------|
| `~` | 列出執行緒 |
| `~s` | 切換到執行緒 |
| `~*k` | 顯示所有執行緒堆疊 |

---

## 四、條件式斷點

### 建立條件斷點

```powershell
# 當變數等於某值時觸發
bp myapp!func "r @rcx = 0n123; .if(@rcx=0n123) {} .else {gc}"

# 使用 j 指令
bp /w "@rcx == 0n123" myapp!func
```

### 指令腳本

```powershell
# 建立斷點並執行指令
bp myapp!MyFunction "r; k; gc"
```

---

## 五、無法使用 WinDbg 時的替代方案

### 1. Visual Studio (最佳替代)

| 功能 | 說明 |
|------|------|
| F9 | 設定/移除斷點 |
| F5 | 開始偵錯 |
| F10 | 逐步執行 |
| F11 | 進入函數 |
| Shift+F11 | 離開函數 |
| 條件中斷點 | 右鍵 > 條件 |
| 函數中斷點 | 中斷點視窗 > 新增 > 函數斷點 |

### 2. VS Code (跨平台)

```json
{
    "configurations": [
        {
            "name": "C++ (Windows)",
            "type": "cppvsdebug",
            "request": "launch",
            "program": "${workspaceFolder}/a.exe",
            "args": [],
            "stopAtEntry": true
        }
    ]
}
```

### 3. dnSpy / ILSpy (.NET 專用)

- 完全免費開源
- 支援 .NET Framework / .NET Core / .NET 5+
- 來源層級偵錯

### 4. JetBrains Rider

- 付費但功能強大
- 支援 .NET / Unity
- 現代化 UI

### 5. Debugging Tools for Windows (CDB/NTSD)

- 命令列版本
- 與 WinDbg 相同核心
- 適合自動化腳本

---

## 六、斷點對應原理

### 需要檔案

| 檔案 | 用途 |
|------|------|
| **PDB** | 符號資料庫 (必須) |
| **原始碼** | 顯示原始碼 (可選) |
| **EXE/DLL** | 目標程式 |

### 符號伺服器

```powershell
# 設定微軟符號伺服器
.sympath srv*C:\symbols*https://msdl.microsoft.com/download/symbols

# 設定專案符號
.sympath+ C:\myproject\debug
```

---

## 七、實用範例

### 攔截函數呼叫

```powershell
# 在函數上設定斷點
bp myapp!MyClass::Method

# 顯示呼叫參數
bp myapp!func "r; gc"
```

### 攔截 API 呼叫

```powershell
# 攔截 CreateFile
bp kernel32!CreateFileW "r; gc"

# 攔截所有 LoadLibrary
bp kernel32!LoadLibraryW "kp; gc"
```

### 記憶體存取中斷點

```powershell
# 當位址被寫入時中斷
ba w4 0x00123456
```

---

## 官方資源

| 資源 | 連結 |
|------|------|
| 微軟官方教學 | https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-breakpoints-in-windbg |
| WinDbg 指令參考 | https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/ |
| 符號設定 | https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/using-symbols |

---

*建立時間: 2026-03-25*
