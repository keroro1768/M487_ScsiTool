# 驅動程式問題與解決

## 🔍 問題診斷流程

```
1. OpenOCD 找不到 Nu-Link
   → 檢查 USB 連接
   → 檢查驅動狀態（裝置管理員）
   → 用 Zadig 更換驅動

2. OpenOCD LIBUSB_ERROR_ACCESS
   → 驅動為 HID 或 libusb-win32（非 WinUSB）
   → 用 Zadig 換成 WinUSB
   → 嘗試以 admin 執行

3. Keil 無法燒錄
   → 確認 Nu-Link 在 Keil 中被識別
   → 確認 Pack Installer 有安裝 M4 DFP
```

---

## 📊 Nu-Link 驅動狀態對照

| 介面 | 預期驅動 | 識別方式 |
|------|----------|----------|
| Interface 0 (HID) | HID（系統內建） | 裝置管理員顯示「HID 相容裝置」 |
| Interface 1 (WinUSB) | WinUSB (Nuvoton) | 裝置管理員顯示「Nuvoton Nu-Link USB」 |

---

## 🔧 Zadig 驅動置換步驟

### Zadig 下載

- 官方：https://zadig.akeo.ie/
- 本地：`C:\Users\rinry\Tool\zadig-2.9.exe`

### 置換流程

1. **開啟 Zadig**（管理員身份）
2. `Device` → `Load Preset Device`
3. 選 `NULink.preset`（或手動輸入 VID=0x0416, PID=0x511C, MI=00）
4. Driver 選 `WinUSB (v6.x.x.x)`
5. 點 `Replace Driver`（或 `Install Driver`）
6. 出現 UAC 提示 → 點 `是`
7. 等待綠色 WinUSB 文字出現

### 預設檔（NULink.preset）

```ini
[Preset Device]
Name=Nuvoton Nu-Link CMSIS-DAP
VID=0x0416
PID=0x511C
MI=00
Driver=WinUSB
```

---

## 🔄 恢復原有驅動

若需要恢復為 HID 驅動：
1. 裝置管理員 → 找到 `Nuvoton Nu-Link USB`
2. 滑鼠右鍵 → `更新驅動程式` → `回復驅動程式`

---

## ⚠️ Windows USB 權限問題

### LIBUSB_ERROR_ACCESS

**原因**：非 admin 執行，無法訪問 WinUSB 裝置

**解決方案**：
1. 用 admin 身份執行 OpenOCD
2. 或將使用者加入 `libusb-win32` 群組（不推薦）
3. 或使用 Windows 認證的 WinUSB 驅動（Zadig 已做）

### LIBUSB_ERROR_NOT_FOUND

**原因**：Nu-Link 未連接或驅動未正確安裝

**解決**：
1. 重新插拔 Nu-Link USB
2. 確認 Interface 1 在裝置管理員中正常

---

## 📋 驅動安裝歷史（2026-03-26）

| 時間 | 操作 | 結果 |
|------|------|------|
| 初始狀態 | Interface 0: HID, Interface 1: WinUSB (oem132.inf) | ✅ OpenOCD 可用 |
| Zadig libusb-win32 | 嘗試安裝 libusb-win32 | ⚠️ OpenOCD Bulk 傳輸失敗 |
| 恢復 WinUSB | Zadig 換回 WinUSB | ✅ 正常 |
| 安裝 Nu-Link_Keil_Driver_V3.22 | Nuvoton 原廠驅動 | ✅ Interface 1: Nuvoton Nu-Link USB |

---

## 🔧 PowerShell 檢查腳本

```powershell
# 檢查 Nu-Link 狀態
Get-PnpDevice | Where-Object { $_.DeviceID -like '*0416*511C*' } |
    Format-Table FriendlyName, Status, InstanceId

# 檢查驅動程式
Get-WmiObject -Class Win32_PnPSignedDriver |
    Where-Object { $_.DeviceID -like '*0416*511C*' } |
    Select-Object DeviceName, DriverVersion, InfName
```

---

## 💡 驅動程式供应商

| 驅動 | 供應商 | 用途 |
|------|--------|------|
| HID (input.inf) | Microsoft | 系統內建 |
| WinUSB (oem132.inf) | Nuvoton | Nu-Link Interface 1 |
| libusb-win32 | libusb project | 通用 USB 訪問 |
