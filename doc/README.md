# Documents — 文件總覽

> 所有文件的集中管理區

## 📁 目錄結構

```
doc/
├── README.md                    ← 本說明文件
│
├── M487/                      ← M487 晶片相關
│   └── M487_Correct_Usage_Guide.md
│
├── NuLink/                    ← Nu-Link 燒錄器相關
│   └── NuLink_Experience_Compilation.md
│
├── hardware/                  ← 硬體相關文件
│   ├── ARM-Cortex-M4/         ← ARM 核心知識
│   ├── I3C-USB-Bridge/       ← I3C 橋接研究
│   └── M487/                  ← M487 硬體手冊、工具
│
├── HID-over-I2C/             ← HID-over-I2C 協定
│   ├── 01-Spec/              ← 規格文件
│   ├── 02-Architecture/      ← 架構設計
│   ├── 03-Protocol/          ← 協定翻譯層
│   ├── 04-Registers/         ← 寄存器定義
│   ├── 05-Example/           ← 範例
│   ├── 06-TestPlan/          ← 測試計劃
│   └── 07-Review/            ← Code Review
│
├── design/                    ← 設計文件
│   └── UI-UX-Design/         ← UI/UX 設計
│
├── DWT/                       ← DWT 研究
│
├── GDB_RSP/                   ← GDB RSP 研究
│
├── IDE_Setup/                  ← IDE 設定指南
│   └── VSCode_OpenOCD_GDB_M487_IDE_Setup.md
│
└── USB_Driver_Samples/        ← USB Driver Samples 研究
```

## 📋 主題分類

| 主題 | 路徑 |
|------|------|
| M487 硬體/晶片 | `hardware/M487/` |
| M487 使用方法 | `M487/` |
| HID-over-I2C 協定 | `HID-over-I2C/` |
| I3C 橋接研究 | `hardware/I3C-USB-Bridge/` |
| ARM Cortex-M4 知識 | `hardware/ARM-Cortex-M4/` |
| IDE 設定 | `IDE_Setup/` |
| Debug 研究 | `DWT/`, `GDB_RSP/` |
| USB Driver | `USB_Driver_Samples/` |
| Nu-Link 燒錄器 | `NuLink/` |
| UI/UX 設計 | `design/UI-UX-Design/` |
