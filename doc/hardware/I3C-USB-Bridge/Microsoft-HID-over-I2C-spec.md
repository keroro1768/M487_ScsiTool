# HID Over I2C Protocol Specification v1.0

> Microsoft Official Document
> Version: 1.0
> Source: docs/pdf/hid-over-i2c-protocol-spec-v1-0.docx

---

## 1. Introduction

HID (Human Interface Device) protocol was first defined for USB attached input devices (e.g., keyboards, mice, remote controls, buttons, etc.). The protocol itself is bus agnostic and has been ported across other transports, including Bluetooth and other wired and wireless technologies.

This specification defines the protocol, procedures, and features for simple input devices to talk HID over I2C.

---

## 2. Definitions

| Term | Definition |
|------|------------|
| **HID** | Human Interface Device - refers to the device itself |
| **I2C** | Inter-Integrated Circuit - multi-master serial single-ended bus |
| **SPB** | Simple Peripheral Bus (I2C, SPI) |
| **USB** | Universal Serial Bus |
| **HOST** | I2C controller or OS software governing the controller |
| **DEVICE** | I2C peripheral compliant with HID over I2C spec |

---

## 3. Architecture

## USB – [Universal Serial Bus] Universal Serial Bus (USB) is an industry standard which defines the cables, connectors and protocols used for connection, communication and power supply between USB compliant Hosts and Devices.

## This document is intended to provide a set of general and unambiguous rules for implementing the HID protocol over I²C for a DEVICE, with the goals of hardware software compatibility (including software reuse), performance and power management.

## The HID protocol is an asymmetric protocol that identifies roles for the HOST and the DEVICE. The protocol will define a format (Descriptors) for the DEVICE to describe its capabilities to the HOST. Once the HOST understands the format of communication with the DEVICE, it programs the DEVICE for sending data back to the HOST. The HID protocol also identifies ways of sending data to the DEVICE as well as status checks for identifying the current state of the device.

## Scenarios – A brief description of the potential scenarios and the goals of this specification to address existing problems around these scenarios

## Descriptors – A summary of the data elements that will be exchanged between the HOST and the DEVICE to enable enumeration and device identification.

## Reports – A summary of data elements that will be exchanged between the HOST and the DEVICE to enable transfer of data in the form of INPUT and OUTPUT reports.

## Requests – A summary of the commands and response between the HOST and the DEVICE.

## Power Management and Error Correction – A summary on the different commands that are sent from HOST to DEVICE to set/get state information and to address any protocol errors that may occur over the selected transport.

## The Appendix section provides examples and for a specific device end to end.

## Scenarios

---

## 4. Key Features

### 4.1 Device Enumeration
- HID descriptors exchanged between HOST and DEVICE
- Device identification and configuration

### 4.2 Data Transfer
- INPUT Reports: Device → Host
- OUTPUT Reports: Host → Device
- Feature Reports: Configuration data

### 4.3 Power Management
- Low-power operation support
- Power state management commands

### 4.4 Error Correction
- Protocol error detection and recovery
- Status checking mechanisms

---

## 5. Comparison with USB HID

| Feature | USB HID | HID over I2C |
|---------|---------|---------------|
| Speed | 12-480 Mbps | 400 Kbps (Fast I2C) |
| Power | Higher | Lower (designed for mobile) |
| Driver | Built-in | Built-in (Windows 8+) |
| Complexity | Medium | Simple |

---

## 6. Use Cases

- **Touchpads**: Laptop touch controllers
- **Keyboards**: Keyboard matrices
- **Remote Controls**: IR remote controllers
- **Sensors**: Environmental sensors
- **Barcode Readers**: Input devices

---

## 7. Reference

- Official Docx: [hid-over-i2c-protocol-spec-v1-0.docx](docs/pdf/hid-over-i2c-protocol-spec-v1-0.docx)
- Microsoft Download: https://download.microsoft.com/download/7/D/D/7DD44BB7-2A7A-4505-AC1C-7227D3D96D5B/hid-over-i2c-protocol-spec-v1-0.docx

---

*This is a summary. Full specification available in the DOCX file.*
