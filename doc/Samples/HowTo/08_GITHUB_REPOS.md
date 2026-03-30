# GitHub Repo 整理

## 🔑 GitHub 帳號

- **Username**: keroro1768
- **Access Token**: 請參考 `C:\Users\rinry\.openclaw\workspace\TOOLS.md` 中的 GitHub Access Token 欄位

---

## 📂 已 Clone 的 Repo

### M480BSP

| 項目 | 內容 |
|------|------|
| **URL** | https://github.com/OpenNuvoton/M480BSP |
| **本地位置** | `D:\AiWorkSpace\KM\M480BSP` |
| **內容** | 驅動程式庫、範例程式（含 USB HSMSC）、Header 檔 |
| **大小** | ~646MB |

### Nu-Link2-Bridge_Firmware

| 項目 | 內容 |
|------|------|
| **URL** | https://github.com/OpenNuvoton/Nu-Link2-Bridge_Firmware |
| **本地位置** | `D:\AiWorkSpace\KM\M487-Examples\Nu-Link2-Bridge_Firmware` |
| **內容** | USB to UART/I2C/SPI/CAN 橋接範例 |

### OpenOCD-Nuvoton（預設在 Tool）

| 項目 | 內容 |
|------|------|
| **URL** | https://github.com/OpenNuvoton/OpenOCD-Nuvoton |
| **版本** | v1.02.029r |
| **位置** | `C:\Users\rinry\Tool\OpenOCD-Nuvoton` |

---

## 📋 KM-Collect Repo

| 項目 | 內容 |
|------|------|
| **URL** | https://github.com/keroro1768/KM-collect |
| **本地位置** | `D:\AiWorkSpace\KM\KM-collect` |
| **內容** | M487 文件、驅動、工具鏈記錄 |

---

## ⚠️ Token 安全提醒

> ⚠️ **請勿將真實 GitHub PAT 寫入文件！** 所有 Token 請存放在 `TOOLS.md` 中，不要 commit 到 Git。

**建議 Token 管理方式：**
1. 使用 Fine-grained PAT，設定過期時間
2. 將 Token 存放在 `TOOLS.md` 中（不在 Git 追蹤範圍）
3. 切勿將 Token 直接寫入 .md 文件

---

## 📥 Git Clone 指令

```bash
# M480BSP（含完整驅動和範例）
git clone https://github.com/OpenNuvoton/M480BSP.git D:\AiWorkSpace\KM\M480BSP

# Nu-Link2 Bridge Firmware
git clone https://github.com/OpenNuvoton/Nu-Link2-Bridge_Firmware.git

# OpenOCD-Nuvoton
git clone https://github.com/OpenNuvoton/OpenOCD-Nuvoton.git C:\Users\rinry\Tool\OpenOCD-Nuvoton

# KM-Collect
git clone https://github.com/keroro1768/KM-collect.git D:\AiWorkSpace\KM\KM-collect
```

---

## 🔧 Git 設定

```bash
# 設定使用者資訊
git config user.email "your@email.com"
git config user.name "keroro1768"

# 設定遠端 URL（包含 Token，Token 請從 TOOLS.md 取得）
git remote set-url origin https://keroro1768:<TOKEN>@github.com/keroro1768/<REPO>.git
```
