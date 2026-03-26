# HID-over-I²C Bridge Documentation

> Branch: `architecture/hid-over-i2c`  
> Created: 2026-03-26

---

## Document Index

### 📋 Specification
- [`01-Spec/SPEC.md`](01-Spec/SPEC.md) — HID over I²C Protocol Specification Summary (from Microsoft spec v1.0)

### 🏗️ Architecture
- [`02-Architecture/ARCHITECTURE.md`](02-Architecture/ARCHITECTURE.md) — M487 Bridge Architecture Design

### 🔄 Protocol
- [`03-Protocol/TRANSLATION.md`](03-Protocol/TRANSLATION.md) — USB ↔ I²C Translation Protocol Details

### 📝 Registers
- [`04-Registers/REGISTERS.md`](04-Registers/REGISTERS.md) — Firmware Register Map & Data Structures

### 💡 Examples
- [`05-Example/EXAMPLE.md`](05-Example/EXAMPLE.md) — Example Device Integration (MLX90614 IR Sensor)

### 🧪 Test Plan
- [`06-TestPlan/TEST_PLAN.md`](06-TestPlan/TEST_PLAN.md) — Testing Strategy & Test Cases

### 🏆 Review
- [`07-Review/REVIEW.md`](07-Review/REVIEW.md) — Review Checklist, Report Template & Process

---

## Quick Reference

### System Architecture
```
PC (USB HID Host) ←USB→ M487 Bridge ←I²C→ HID-over-I²C Device
```

### Key Documents
| Topic | Document |
|-------|----------|
| Protocol details | `03-Protocol/TRANSLATION.md` |
| Register definitions | `04-Registers/REGISTERS.md` |
| Test strategy | `06-TestPlan/TEST_PLAN.md` |
| Review process | `07-Review/REVIEW.md` |

---

## Status

- [x] SPEC.md — Complete
- [x] ARCHITECTURE.md — Complete
- [x] TRANSLATION.md — Complete
- [x] REGISTERS.md — Complete
- [x] EXAMPLE.md — Complete
- [x] TEST_PLAN.md — Complete
- [x] REVIEW.md — Complete
- [ ] **Implementation** — Pending

---

## Review Policy

**每個實作階段完成後必須 Review，產生報告，再依據報告修改。**

- **程式碼 Review：** 每個模組實作完成
- **整合 Review：** 模組整合後
- **最終 Review：** 交付前

詳見 [`07-Review/REVIEW.md`](07-Review/REVIEW.md)

---

## Implementation Roadmap

1. **Phase 1:** I²C driver (polling mode, basic read/write)
2. **Phase 2:** HID Descriptor parser
3. **Phase 3:** USB HID device layer (enumeration, EP0, EP1, EP2)
4. **Phase 4:** Translation layer (command builder, response parser)
5. **Phase 5:** Integration & testing

**每個 Phase 完成後 → Review → 產生報告 → 修復問題 → 下一 Phase**
