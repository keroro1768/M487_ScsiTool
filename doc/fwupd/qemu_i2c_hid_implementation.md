# QEMU + I2C-HID 實作規劃

**日期：** 2026-03-30  
**目標：** 在 x86_64 Ubuntu 上建立 QEMU ARM virt 環境，模擬 I2C-HID 設備用於 FWUPD 測試  
**最終目標：** 實際韌體燒錄驗證

---

## 1. 為何選擇 QEMU ARM virt

| 方案 | 優點 | 缺點 |
|------|------|------|
| **QEMU ARM virt** | DTB 支援完整、文件齊全、最接近真實硬體 | 需要跑 ARM VM |
| x86_64 QEMU | 可直接用 | ACPI 代替 DTB，更複雜 |
| 用戶空間 I2C 模擬 | 簡單 | 繞過真實 I2C 棧，FWUPD 可能不識別 |

**結論：** ARM virt + DTB 是最接近真實 HID-over-I2C 的模擬方案。

---

## 2. 系統架構

```
┌─────────────────────────────────────────────────────────────┐
│  x86_64 Ubuntu Host                                          │
│                                                              │
│  ┌─────────────────┐    ┌─────────────────────────────┐     │
│  │  QEMU System    │    │  Network (SSH, fwupdmgr)    │     │
│  │  ARM64 (virt)   │    │  port 2222 → VM:22          │     │
│  │                 │    └─────────────────────────────┘     │
│  │  ┌───────────┐  │                                        │
│  │  │  Linux    │  │                                        │
│  │  │  Kernel   │  │                                        │
│  │  │  + DTB    │  │                                        │
│  │  │  + i2c-hid│  │                                        │
│  │  │  + fwupd  │  │                                        │
│  │  └───────────┘  │                                        │
│  └─────────────────┘                                        │
└─────────────────────────────────────────────────────────────┘
```

---

## 3. 實作階段

### Phase 1：環境建立 ✅ 已規劃

```
tool/qemu_i2c_hid/setup.sh          # 主要安裝腳本
tool/qemu_i2c_hid/create_dtb_overlay.sh  # DTB overlay 生成
```

**執行：**
```bash
# 在 Ubuntu 上執行
chmod +x tool/qemu_i2c_hid/setup.sh
./tool/qemu_i2c_hid/setup.sh all
```

### Phase 2：DTB 注入（待完成）

需要在 QEMU 啟動時載入自定義 DTB，包含 I2C-HID 設備節點。

**關鍵 DTB 欄位：**
```dts
&i2c0 {
    status = "okay";
    
    virt-hid@15 {
        compatible = "hid-over-i2c";
        reg = <0x15>;           // I2C 地址
        hid-descr-addr = <0x1F>; // HID descriptor 地址
        interrupts-extended = <&gic ...>;
        vid = <0x04F3>;
        pid = <0x0732>;
    };
};
```

### Phase 3：Custom QEMU Device（可選，高難度）

如果 DTB + i2c-hid 驅動無法自動識別，需要自定義 QEMU 設備模型。

**参考：** `qemu_i2c_hid_plan.md` Section 8

---

## 4. 已知限制

| 問題 | 影響 | 解法 |
|------|------|------|
| QEMU 無原生 I2C-HID | i2c-hid 驅動可能無法識別 | 實現 Custom Device Model |
| FWUPD 需要正確 HID_ID | 可能識別失敗 | 調整 DTB vid/pid 或 fwupd 設定 |
| HID Report 交換需要實際設備 | 只做枚舉測試可以 | 實現完整 device model |

---

## 5. 下一步

1. **在 Ubuntu 上執行 setup.sh**
2. **確認 QEMU ARM VM 能啟動**
3. **驗證 i2c-hid 模組在 VM 內可用**
4. **再研究 Phase 2 DTB 注入**

---

## 6. 替代方案随選

如果 QEMU ARM 路徑太複雜，可以考慮：

1. **fwupdtool emulation** - 測試 FWUPD 邏輯（推薦快速驗證）
2. **真實硬體** - M487 燒錄測試（最終目標）

---

**更新時間：** 2026-03-30 12:52
