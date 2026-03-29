# T033 研究：GDB RSP Server

## 協議基礎

GDB Remote Serial Protocol (RSP) 基於 ASCII 字元封包，典型傳輸：- RS-232 或 TCP socket
- 速率：9600~115200 baud（UART）或 USB MSC bulk endpoint

## 核心 GDB RSP Command

| Command | Direction | Description |
|---------|-----------|-------------|
| `g` | Host→Target | 讀取所有 CPU 通用暫存器（R0-R12, SP, LR, PC, xPSR）|
| `G` | Host→Target | 寫入所有 CPU 通用暫存器 |
| `p n` | Host→Target | 讀取第 n 號暫存器 |
| `P n=r` | Host→Target | 寫入第 n 號暫存器為值 r |
| `m addr,len` | Host→Target | 讀取記憶體（addr:hex, len:hex）|
| `M addr,len:data` | Host→Target | 寫入記憶體 |
| `c` | Host→Target | 繼續執行 |
| `s` | Host→Target | 單步執行（Step）|
| `z type,addr,kind` | Host→Target | 刪除 breakpoint/watchpoint |
| `Z type,addr,kind` | Host→Target | 設定 breakpoint/watchpoint |
| `?` | Host→Target | 查詢目前 signal（T01=HALTED）|
| `Ctrl+C` | Host→Target | 中斷（送出 0x03，Target 需中斷執行）|

## Target Response

- `OK` = 成功
- `E nn` = 錯誤（nn 為十六進位錯誤碼）
- `S nn` = Signal（T01=HALT, T02=STEP, etc.）
- `T nn` = Stop reply with regs（STOP REPLY）
- `+` / `-` = ACK/NACK（可用校驗）

## Arm Cortex-M Halt Mode

### 進入 Halt 方式

1. **Debug Access Port (DAP)**：JTAG/SWD → 寫入 DHCSR.C_DEBUGEN = 1
2. **BKPT 指令**：執行程式時遇到 `BKPT #n` → 進入 DebugMonitor
3. **Fault**：HardFault / MemManage Fault 可設為 fault trap
4. **DWT Trigger**：PC match / data watchpoint 觸發時自動 halt

### Halt 後可做的事

- 讀寫所有 CPU 暫存器（R0-R15, xPSR, MSP, PSP, CONTROL, PRIMASK, etc.）
- 讀寫 SRAM（任意位址）
- 讀取 Flash（唯讀，需 FMC）
- 設定硬體 breakpoint（最多 6 個 via FP_CTRL）
- 設定 watchpoint（最多 4 個 via DWT）

### 離開 Halt

- `main()` 迴圈中斷
- `HSUSBD_Start()` 還沒被呼叫 → USB 未啟動

## MSC Debug Channel 整合方案

### 方案 A：在 MSC_VendorCommand 加入 RSP Command

```c
// CDB[1] = 0x10 ~ 0x1F 保留給 RSP
case 0x10: // RSP: 'g' read all regs
case 0x11: // RSP: 'G' write all regs
case 0x12: // RSP: 'p n' read one reg
case 0x13: // RSP: 'P n=val' write one reg
case 0x14: // RSP: 'm addr,len' read mem
case 0x15: // RSP: 'M addr,len:data' write mem
case 0x16: // RSP: 'c' continue
case 0x17: // RSP: 's' step
case 0x18: // RSP: '?' stop reason
case 0x1F: // RSP: Ctrl+C interrupt
```

**問題**：主迴圈只有 `MSC_ProcessCmd()`，無中斷驅動的 RSP listener。需要輪詢 MSC command。

### 方案 B：獨立的 RSP Task（RTOS 或 Main Loop）

```c
while (1) {
    // 處理 MSC command
    MSC_ProcessCmd();
    
    // 處理 RSP command（如果有）
    if (g_u8RspReady) {
        Rsp_Process();
    }
}
```

### 方案 C：透過 USB MSC BOT 的 SCSI CDB 封裝

MSC Debug Channel 已經是 CDB 0xC0-0xCF range。可以擴展：
- CDB 0xD0 = RSP packet data（host → device）
- CDB 0xD1 = RSP packet data（device → host）

Data stage 直接承載 RSP 封包內容，繞過 CDB 格式限制。

## 關鍵障礙：MSC BOT Protocol

MSC BOT 的問題：
1. CBW 固定 31 bytes（data direction, length, CDB[16]）
2. Data stage 要嘛 IN 要嘛 OUT，不能同時雙向
3. RSP `c` (continue) 和 `g` (read regs) 之類的命令需要多次來回

**結論**：MSC Debug Channel 適合讀取狀態，不適合承載完整的 RSP 互動。

## 建議的 Debug 流程（回到辦公室後）

1. **OpenOCD + GDB**：現有電路，已驗證可用
2. **DWT + ITM**：T024 已完成，SWO 可追蹤
3. **MSC Debug CLI**：`msc_debug.exe` 讀取記憶體/暫存器（不需要 halt）

RSP Server 的價值在於「不需要 OpenOCD」但 MSC Debug Channel 已經提供了等效功能。

## 文件位置

- RSP Spec: `https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html`
- ARM ARM: `DWT` 章節（Cortex-M4 TRM）
- M487 DWT: 在 `Cortex-M4 with FPU` core，M487 RM CH20
