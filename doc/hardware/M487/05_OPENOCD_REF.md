# OpenOCD 指令參考

## 基本語法

```bash
openocd.exe -s <scripts_dir> -f <config.cfg> [-c "command"] [-c "command"]
```

## 常用參數

| 參數 | 說明 |
|------|------|
| `-s <dir>` | 指定 scripts 目錄 |
| `-f <file>` | 載入設定檔 |
| `-c "cmd"` | 執行 OpenOCD 指令 |
| `-d <level>` | Debug 層級 (0-3) |

---

## 初始化與連線

```tcl
# 初始化 Debug 介面
init

# 列出所有 targets
targets

# 連線到特定 target
target 0
```

---

## CPU 控制

```tcl
# Halt CPU
halt

# Reset 並 halt
reset halt

# 軟體 Reset
reset run

# 繼續執行
resume

# 查看暫存器
reg pc
reg msp
reg psp
```

---

## Flash 操作

```tcl
# 讀取 Flash
flash read_bank 0 <file> <offset> <size>

# 燒錄 Flash (自動抹除)
flash write_image erase <file> <offset>

# 只燒錄不抹除
flash write_image <file> <offset>

# 抹除特定範圍
flash erase_address <addr> <size>

# 抹除整個晶片
mflash erase <bank>
```

---

## Nu-Link 特定設定

```tcl
# 指定 Nu-Link
interface nulink

# SWD 傳輸 (自動選擇)
transport select hla_swd

# 設定時脈 (kHz)
adapter speed 1000
```

---

## 實用範例

### 完整燒錄流程

```tcl
init
reset halt
flash write_image erase C:/Users/rinry/m487_flash.bin 0
shutdown
```

### 完整讀取流程

```tcl
init
reset halt
flash read_bank 0 C:/Users/rinry/m487_flash.bin 0 0x80000
shutdown
```

### 讀取並顯示記憶體

```tcl
init
reset halt
mdw 0x00000000 16
shutdown
```

### 燒錄後驗證

```tcl
init
reset halt
# 先燒錄
flash write_image erase C:/firmware.bin 0
# 再讀回比對
flash read_bank 0 C:/verify.bin 0 0x80000
shutdown
```

---

## 批次執行

```bash
# 使用命令列
openocd.exe -c "init" -c "reset halt" -c "flash read_bank 0 backup.bin 0 0x80000" -c "shutdown"
```

---

## Debug 等級

| 等級 | 輸出量 |
|------|--------|
| `-d 0` | 只有 Error |
| `-d 1` | Error + Warning |
| `-d 2` | Error + Warning + Info |
| `-d 3` | Error + Warning + Info + Debug |

---

## TCL 腳本

### 讀取腳本範例 (read_m487_flash.tcl)

```tcl
set flash_size 0x80000
set chunk_size 0x10000
set total_chunks [expr {$flash_size / $chunk_size}]
set filename "C:/Users/rinry/m487_flash.bin"

echo "=== M487 Flash Read: $total_chunks chunks ==="

for {set i 0} {$i < $total_chunks} {incr i} {
    set offset [expr {$i * $chunk_size}]
    echo [format "Reading chunk %d/%d (offset 0x%08x)..." $i $total_chunks $offset]
    flash read_bank 0 $filename $offset $chunk_size
    echo [format "Chunk %d/%d done." $i $total_chunks]
}

echo "=== Complete ==="
shutdown
```

---

## 疑難排解

| 錯誤 | 原因 | 解決 |
|------|------|------|
| LIBUSB_ERROR_ACCESS | 權限不足 | 用 admin 執行或確認驅動 |
| couldn't open file | 路徑錯誤 | 確認目錄存在 |
| unknown command | 指令錯誤 | 檢查 TCL 語法 |
