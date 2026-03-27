# WinDbg 官方教學完整指南

## 來源
- Microsoft Learn: https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/
- WinDbg 官方文檔

---

## 一、WinDbg 簡介

WinDbg 是 Microsoft 提供的強大 Windows 除錯工具，用於：
- 使用者模式 (User-Mode) 應用程式偵錯
- 核心模式 (Kernel-Mode) 驅動程式偵錯
- 崩潰分析 (Crash Dump Analysis)
- 自動化腳本

---

## 二、開始使用 WinDbg

### 安裝方式

#### 方法1：透過 Windows 應用商店
```
應用程式 > Microsoft WinDbg (Preview)
```

#### 方法2：透過 SDK
```
Windows SDK - Debugging Tools for Windows
```

---

## 三、啟動除錯會話

### 1. 附加到執行中程序

```
File > Attach to Process > 選擇程序
```

### 2. 啟動新程序

```
File > Launch Executable (advanced)
```

### 3. 核心模式除錯

```
File > Kernel Debug
```

---

## 四、斷點設定 (官方)

### 方法一：Debugger Command Window

在命令視窗輸入指令設定斷點：
```
bp myapp!FunctionName
```

### 方法二：選單操作

1. **Edit > Breakpoints** 
2. 或按 **ALT+F9**
3. 對話框可管理所有斷點

### 方法三：程式碼視窗

- 在 **Disassembly** 視窗
- 在 **Source** 視窗
- 將游標放在目標行，按 **F9**

### 斷點顯示

| 顏色 | 狀態 |
|------|------|
| 紅色 | 已啟用斷點 |
| 黃色 | 已停用斷點 |

---

## 五、斷點類型

### 1. 軟體斷點 (bp)

```
# 在函數設斷點
bp myapp!MyClass::Method

# 在位址設斷點
bp 0x00401000

# 原始碼行號
bp /l:25 main.cpp
```

### 2. 模組符號斷點 (bm)

```
# 符合模式的所有符號
bm myapp!*

# 特定模式
bm myapp!Button_*
```

### 3. 未解析符號斷點 (bu)

```
# 延遲載入的符號
bu mydll!DeferredFunction
```

### 4. 記憶體存取斷點 (ba)

```
# 位址被寫入時中斷
ba w4 0x00123456

# 位址被讀取時中斷
ba r4 0x00123456

# 執行時中斷
ba e4 0x00123456
```

---

## 六、執行控制指令

### 基本執行

| 指令 | 按鍵 | 說明 |
|------|------|------|
| `g` | F5 | 執行 (Go) |
| `p` | F10 | 逐步執行 (不進入函數) |
| `t` | F11 | 逐步執行 (進入函數) |
| `gu` | Shift+F11 | 執行到函數結束 |
| `pc` | | 執行到下一個函數呼叫 |
| `pa` | | 執行到位址 |

### 中斷執行

| 指令 | 說明 |
|------|------|
| `Ctrl+Break` | 中斷執行 |
| `sxe` | 啟用例外中斷 |
| `sxd` | 停用例外中斷 |

---

## 七、顯示與檢視

### 暫存器

```
r                       # 顯示所有暫存器
r eax                  # 顯示特定暫存器
r @rax=0x10          # 修改暫存器
```

### 記憶體

| 指令 | 說明 |
|------|------|
| `db address` | 顯示位元組 (Hex+ASCII) |
| `dw address` | 顯示 Word |
| `dd address` | 顯示 Double Word |
| `dq address` | 顯示 Quad Word |
| `da address` | 顯示 ASCII 字串 |
| `du address` | 顯示 Unicode 字串 |

### 呼叫堆疊

```
k                       # 基本堆疊
kv                      # 詳細堆疊 (含 Frame 指標)
kp                      # 含參數資訊
```

### 執行緒

```
~                      # 列出所有執行緒
~s                     # 切換到目前執行緒
~*k                    # 所有執行緒堆疊
```

---

## 八、模組與符號

### 載入模組

```
lm                      # 列出所有模組
lm m moduleName        # 搜尋模組
```

### 符號路徑

```
# 設定符號路徑
.sympath srv*C:\symbols*https://msdl.microsoft.com/download/symbols

# 加入專案符號
.sympath+ C:\myproject\debug
```

### 符號解析

```
x myapp!*              # 列出模組所有符號
ln address             # 列出附近的符號
```

---

## 九、自動化腳本

### 腳本檔案

```powershell
# commands.txt
r
k
lm
g
```

執行腳本：
```
$$< commands.txt
```

### 條件斷點

```
# 當條件成立時中斷
bp /w "@rax == 0x1234" myapp!func
```

---

## 十、常見使用情境

### 1. 分析程式崩潰

```
# 開啟Dump檔
windbg -z dump.dmp

# 查看崩潰原因
!analyze -v
```

### 2. 附加到程序

```
# 程序 PID
.attach -p 1234

# 或
File > Attach to Process > 選擇 PID
```

### 3. .NET 程式除錯

```
# 載入 .NET 擴充
.loadby sos clr

# 顯示堆疊
!clrstack

# 顯示物件
!dumpheap -stat
```

---

## 十一、WinDbg 快捷鍵

| 按鍵 | 功能 |
|------|------|
| F5 | 執行 |
| F9 | 設定/移除斷點 |
| F10 | 逐步執行 |
| F11 | 進入函數 |
| Shift+F11 | 離開函數 |
| Alt+F9 | 開啟斷點對話框 |
| Ctrl+Break | 中斷執行 |

---

## 十二、官方資源

| 資源 | 連結 |
|------|------|
| 官方首頁 | https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/ |
| 新手教學 | https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/getting-started-with-windbg |
| 指令參考 | https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/ |
| 斷點設定 | https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/setting-breakpoints-in-windbg |
| WinDbg 預覽版 | Microsoft Store 下載 |

---

## 十三、VS Code 替代方案

如果想要更現代的 UI，可以使用：

### VS Code + C++ Debugger

```json
{
    "configurations": [
        {
            "name": "Windows Debug",
            "type": "cppvsdebug",
            "request": "launch",
            "program": "${workspaceFolder}/app.exe",
            "args": [],
            "stopAtEntry": true
        }
    ]
}
```

### 設定 C/C++ 擴充

1. 安裝 C/C++ 擴充
2. 安裝 C++ Debugger (MinGW-w64 / VS Build Tools)
3. 設定 launch.json

---

*整理時間: 2026-03-25*
