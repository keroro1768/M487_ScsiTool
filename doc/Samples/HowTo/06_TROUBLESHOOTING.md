# 疑難排解

---

## 🔌 OpenOCD 問題

### Q: OpenOCD 回應 `LIBUSB_ERROR_ACCESS`

**原因**：USB 驅動權限不足
**解決**：
1. 確認 Nu-Link Interface 1 為 WinUSB
2. 以 admin 執行 OpenOCD
3. 確認 Zadig 已將正確介面設為 WinUSB

### Q: `Error: couldn't open file`

**原因**：輸出路徑無寫入權限或路徑格式錯誤
**解決**：
1. 使用 `/` 而非 `\` 作為路徑分隔符
2. 確認目標目錄存在
3. 確認非 admin 有寫入該目錄的權限

### Q: `Info: Device ID: 0x00d48750` 然後 hang

**原因**：Flash 操作卡住（SWD 傳輸中斷）
**解決**：
1. 按 `Ctrl+C` 中斷
2. 重新插拔 Nu-Link
3. 降低 SWD 時脈（預設 1000 kHz）

---

## 🔧 Keil 編譯問題

### Q: `core_cm4.h file not found`

**原因**：CMSIS 路徑未設定
**解決**：
1. 安裝 `ARM::CMSIS` Pack
2. 在 `Options for Target` → `C/C++` → `Include Paths` 加入：
   ```
   C:\Users\rinry\AppData\Local\Arm\Packs\ARM\CMSIS\6.3.0\CMSIS\Core\Include
   ```

### Q: 相對路徑 `..\..\..\..\Library` 無效

**原因**：BSP 目錄結構與專案不匹配
**解決**：
1. 確認 `M480BSP\` 位於正確位置（專案往上 4 層可達到）
2. 或直接用原範例路徑編譯，不複製到其他位置

### Q: `hsusbd_core.h not found`

**原因**：HSUSBD/USBD 程式庫路徑未設
**解決**：
1. 在 `Include Paths` 加入：
   ```
   C:\Users\rinry\AppData\Local\Arm\Packs\Nuvoton\NuMicroM4_DFP\1.0.1\Device\M480\Include
   ```
2. 或複製 `M480BSP\Library\USBD\` 到專案目錄

---

## 📀 USB MSC 問題

### Q: PC 辨識為「未知裝置」

**原因**：VID/PID 未正確識別，或驅動未安裝
**解決**：
1. 在裝置管理員找到該裝置
2. 用 Zadig 安裝 WinUSB 驅動

### Q: USB 磁碟出現但無法複製檔案

**原因**：`MSD_Write()` 返回 FALSE
**解決**：
1. 確認 `gKeepMediaWriteProtected` 是否為 0
2. 檢查 `STORAGE_DISK_SIZE` 是否超過 SRAM

### Q: 複製進去的檔案讀不到

**原因**：`MSD_Read()` 未正確從 `g_au8StorageDisk[]` 讀取
**解決**：
1. 確認 `MSD_Read()` 實作是從 RAM 讀取而非 dummy 資料
2. 檢查 Flash 燒錄是否成功

---

## 🔄 Nu-Link 斷線問題

### Q: 燒錄時突然斷線

**原因**：
- USB 線鬆脫
- 燒錄時間過長
- 驅動進入休眠

**解決**：
1. 重新插拔 Nu-Link USB
2. 確認 USB 供電充足（建議使用有源的 USB Hub）
3. 嘗試降低燒錄頻率

---

## 💾 Flash 問題

### Q: 讀取 Flash 全是 0xFF

**原因**：
- Flash 為空白（從未燒錄）
- 讀取位址錯誤
- SWD 傳輸失敗

**解決**：
1. 確認燒錄成功（有成功輸出）
2. 嘗試重新燒錄

### Q: 燒錄後無法啟動

**原因**：
- Vector table 未對齊（Flash 位址 0）
- SP/PC 值錯誤
- 燒錄檔损坏

**解決**：
1. 確認 `.bin` 檔案的 vector table 正確
2. 嘗試重新燒錄
3. 用 `reset halt` 確認 CPU 狀態

---

## 📋 快速檢查清單

燒錄前確認以下項目：
- [ ] Nu-Link USB 已連接
- [ ] 裝置管理員顯示 `Nuvoton Nu-Link USB` 正常
- [ ] OpenOCD 可以成功連線（`targets` 指令正常）
- [ ] 燒錄 `.bin` 檔案存在且大小正確
- [ ] M487 目標板供電正常
