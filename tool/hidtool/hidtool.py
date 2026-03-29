#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
hidtool - Unified HID + MSC Debug Channel CLI Tool for Windows
===============================================================

Supports both HID-over-I2C bridge (via hidapi) and MSC Debug Channel
(via Windows SCSI PassThrough) in a single CLI.

Usage:
    hidtool [-h] [-v] [-q] <command> ...

HID Commands:
    hidtool device list                        List all HID devices
    hidtool device select <idx>                Select HID device by index
    hidtool read <i2c_addr> <reg> [len]       I2C read via HID
    hidtool write <i2c_addr> <reg> <data>...  I2C write via HID
    hidtool reg read <reg>                    Read HID bridge register
    hidtool reg write <reg> <data>            Write HID bridge register
    hidtool monitor [interval_ms]              Monitor HID reports (hotplug)

MSC Commands:
    hidtool msc list                           List removable drives (MSC devices)
    hidtool msc select <drive>                 Select MSC device (e.g. E:)
    hidtool msc info                          Read MSC device info
    hidtool msc readmem <addr> <len>          Read memory via MSC debug channel
    hidtool msc writemem <addr> <hex-data>    Write memory via MSC debug channel
    hidtool msc readreg <reg_id>              Read CPU register
    hidtool msc writereg <reg_id> <value>     Write CPU register
    hidtool msc log [count]                   Read debug log entries
    hidtool msc echo <text>                   Echo test

Examples:
    hidtool device list
    hidtool msc info
    hidtool msc readmem 0x20000000 64
    hidtool msc log 32
    hidtool read 0x50 0x10 16
"""

import sys
import os
import ctypes
from ctypes import wintypes
import time
import struct
import argparse
import threading
from typing import Optional, List, Tuple

# ============================================================================
# Windows API Constants (ctypes)
# ============================================================================
INVALID_HANDLE_VALUE = -1
FILE_FLAG_NO_BUFFERING = 0x20000000
GENERIC_READ = 0x80000000
GENERIC_WRITE = 0x40000000
FILE_SHARE_READ = 0x00000001
FILE_SHARE_WRITE = 0x00000002
OPEN_EXISTING = 3
DRIVE_REMOVABLE = 2
IOCTL_SCSI_PASS_THROUGH_DIRECT = 0x4D014

# SCSI PassThrough direction
SCSI_DIR_IN = 1
SCSI_DIR_OUT = 2

# MSC Vendor Protocol
MSC_VENDOR_OPCODE = 0xC0
DBG_GET_INFO = 0x07
DBG_READ_LOG = 0x08
DBG_ECHO = 0x0F
DBG_READ_MEM = 0x01
DBG_WRITE_MEM = 0x02
DBG_READ_REG = 0x03
DBG_WRITE_REG = 0x04

# HID Report IDs
HID_REPORT_ID = 0x01
HID_OP_WRITE = 0x01
HID_OP_READ = 0x02

# ============================================================================
# MSC Structures (packed)
# ============================================================================

class MSC_DeviceInfo(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("u32ChipId", ctypes.c_uint32),
        ("u16FwVersion", ctypes.c_uint16),
        ("u16BuildDate", ctypes.c_uint16),
        ("u32FlashSize", ctypes.c_uint32),
        ("u32RamSize", ctypes.c_uint32),
        ("u8I2cSpeed", ctypes.c_uint8),
        ("u8I2cAddr", ctypes.c_uint8),
        ("u8Reserved", ctypes.c_uint8 * 2),
        ("u32LogHead", ctypes.c_uint32),
        ("u32LogTail", ctypes.c_uint32),
        ("u32LogCount", ctypes.c_uint32),
    ]

class MSC_LogEntry(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("u32Timestamp", ctypes.c_uint32),
        ("u8Level", ctypes.c_uint8),
        ("u8Module", ctypes.c_uint8),
        ("u8Code", ctypes.c_uint8),
        ("u8Len", ctypes.c_uint8),
        ("au8Data", ctypes.c_uint8 * 4),
    ]

class SCSI_PASS_THROUGH_DIRECT_BUFFER_V2(ctypes.Structure):
    _pack_ = 1
    _fields_ = [
        ("au8CDB", ctypes.c_uint8 * 16),
        ("u8CDBLength", ctypes.c_uint8),
        ("u8SenseLength", ctypes.c_uint8),
        ("u8ScsiStatus", ctypes.c_uint8),
        ("bResetSenseInfo", ctypes.c_uint8),
        ("bQueueTagEnable", ctypes.c_uint8),
        ("u16QueueTag", ctypes.c_uint16),
        ("u32QueueAlgorithm", ctypes.c_uint32),
        ("u32DataTransferLength", ctypes.c_uint32),
        ("u32TimeOutValue", ctypes.c_uint32),
        ("pvDataPointer", ctypes.c_void_p),
        ("u32SenseInfoOffset", ctypes.c_uint32),
        ("u8Direction", ctypes.c_uint8),
        ("u8DeviceAddress", ctypes.c_uint8),
        ("u16BusAddress", ctypes.c_uint16),
        ("u32BusType", ctypes.c_uint32),
        ("u32SecurityProtocol", ctypes.c_uint32),
        ("u16SecurityProtocolLength", ctypes.c_uint16),
        ("u32AlignMask", ctypes.c_uint32),
        ("ucToken", ctypes.c_uint8),
        ("dwSystemIoSize", ctypes.c_uint32),
        ("ucSystemIoArch", ctypes.c_uint8),
        ("ucHashForToken", ctypes.c_uint8),
        ("ucReserved38", ctypes.c_uint8),
        ("ucReserved39", ctypes.c_uint8),
    ]

# ============================================================================
# MSC Debug Channel Implementation
# ============================================================================

class MSCDebugChannel:
    """MSC Vendor Debug Channel via Windows SCSI PassThrough"""

    def __init__(self, drive_letter: str):
        self.drive = drive_letter.rstrip('\\')
        self.h_device: Optional[int] = None
        self._open()

    def _open(self):
        dev_name = f"\\\\.\\{self.drive}"
        self.h_device = ctypes.windll.kernel32.CreateFileA(
            dev_name.encode(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            None, OPEN_EXISTING,
            FILE_FLAG_NO_BUFFERING, None
        )
        if self.h_device == INVALID_HANDLE_VALUE:
            raise OSError(f"Cannot open MSC device {self.drive}")

    def close(self):
        if self.h_device and self.h_device != INVALID_HANDLE_VALUE:
            ctypes.windll.kernel32.CloseHandle(self.h_device)
            self.h_device = None

    def _send_command(self, sub_cmd: int, addr: int = 0, data_len: int = 0,
                      data_out: Optional[bytes] = None) -> Tuple[bytes, int]:
        """Send MSC vendor command. Returns (response_data, scsi_status)"""
        cdb = bytes([MSC_VENDOR_OPCODE, sub_cmd,
                     (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF,
                     data_len & 0xFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])

        if data_out:
            transfer_len = len(data_out)
            direction = SCSI_DIR_OUT
            p_data = (ctypes.c_uint8 * transfer_len).from_buffer_copy(data_out)
        else:
            transfer_len = data_len if data_len > 0 else 64
            direction = SCSI_DIR_IN
            p_data = (ctypes.c_uint8 * transfer_len)()

        pt_size = ctypes.sizeof(SCSI_PASS_THROUGH_DIRECT_BUFFER_V2)
        buffer_size = pt_size + transfer_len + 32
        buffer = (ctypes.c_uint8 * buffer_size)()
        p_pt = ctypes.cast(buffer, ctypes.POINTER(SCSI_PASS_THROUGH_DIRECT_BUFFER_V2))[0]

        ctypes.memmove(p_pt.au8CDB, cdb, 16)
        p_pt.u8CDBLength = 6
        p_pt.u8SenseLength = 32
        p_pt.u32DataTransferLength = transfer_len
        p_pt.u32TimeOutValue = 3000
        p_pt.pvDataPointer = ctypes.addressof(p_data)
        p_pt.u32SenseInfoOffset = pt_size
        p_pt.u8Direction = direction

        dw_bytes = wintypes.DWORD()
        result = ctypes.windll.kernel32.DeviceIoControl(
            self.h_device,
            IOCTL_SCSI_PASS_THROUGH_DIRECT,
            ctypes.byref(p_pt), ctypes.sizeof(p_pt),
            ctypes.byref(buffer), buffer_size,
            ctypes.byref(dw_bytes), None
        )

        if not result:
            raise OSError(f"DeviceIoControl failed: {ctypes.windll.kernel32.GetLastError()}")

        scsi_status = p_pt.u8ScsiStatus
        resp = bytes(p_data[:dw_bytes.value]) if direction == SCSI_DIR_IN else b''
        return resp, scsi_status

    def get_info(self) -> MSC_DeviceInfo:
        data, status = self._send_command(DBG_GET_INFO, 0, 0)
        if len(data) < ctypes.sizeof(MSC_DeviceInfo):
            raise ValueError(f"GET_INFO response too short: {len(data)} bytes")
        info = MSC_DeviceInfo()
        ctypes.memmove(ctypes.addressof(info), data[:ctypes.sizeof(MSC_DeviceInfo)], ctypes.sizeof(MSC_DeviceInfo))
        return info

    def read_mem(self, addr: int, length: int) -> bytes:
        if length > 512:
            length = 512
        data, status = self._send_command(DBG_READ_MEM, addr, length)
        return data

    def write_mem(self, addr: int, data: bytes) -> int:
        resp, status = self._send_command(DBG_WRITE_MEM, addr, len(data), data)
        if len(resp) >= 4:
            return struct.unpack('<I', resp[:4])[0]
        return 0

    def read_reg(self, reg_id: int) -> int:
        resp, status = self._send_command(DBG_READ_REG, reg_id, 0)
        if len(resp) >= 4:
            return struct.unpack('<I', resp[:4])[0]
        return 0

    def write_reg(self, reg_id: int, value: int) -> bool:
        data = struct.pack('<I', value)
        resp, status = self._send_command(DBG_WRITE_REG, reg_id, 0, data)
        return len(resp) > 0 and resp[0] == 0

    def read_log(self, count: int = 64) -> List[MSC_LogEntry]:
        if count > 256:
            count = 256
        entry_size = ctypes.sizeof(MSC_LogEntry)
        total = count * entry_size
        data, status = self._send_command(DBG_READ_LOG, 0, total)
        entries = []
        for i in range(count):
            offset = i * entry_size
            if offset + entry_size > len(data):
                break
            e = MSC_LogEntry()
            ctypes.memmove(ctypes.addressof(e), data[offset:offset + entry_size], entry_size)
            entries.append(e)
        return entries

    def echo(self, text: str) -> str:
        b = text.encode('utf-8')[:63]
        resp, status = self._send_command(DBG_ECHO, 0, len(b), b)
        return resp.decode('utf-8', errors='replace')


def find_msc_devices() -> List[str]:
    """Find removable MSC device drive letters"""
    drives = []
    mask = ctypes.windll.kernel32.GetLogicalDrives()
    for i in range(26):
        if mask & (1 << i):
            letter = chr(ord('A') + i)
            path = f"{letter}:\\"
            if ctypes.windll.kernel32.GetDriveTypeW(path) == DRIVE_REMOVABLE:
                drives.append(letter + ":")
    return drives


# ============================================================================
# HID Debug Channel Implementation (via hidapi Python)
# ============================================================================

try:
    import hid as hidapi_module
    HAS_HIDAPI = True
except ImportError:
    HAS_HIDAPI = False


class HIDDebugChannel:
    """HID-over-I2C Bridge communication via hidapi"""

    # Default VID/PID (from SPEC.md)
    DEFAULT_VID = 0x04F3
    DEFAULT_PID = 0x0732
    DEFAULT_USAGE_PAGE = 0xFF00
    DEFAULT_USAGE = 0x01

    def __init__(self):
        self.device = None
        self.selected_index = -1
        self.devices: List[Tuple[int, int, str, str]] = []  # (vid, pid, serial, path)

    def enumerate(self, vid: int = None, pid: int = None) -> List[Tuple[int, int, str, str]]:
        """Enumerate HID devices, optionally filtered by VID/PID"""
        self.devices = []
        if not HAS_HIDAPI:
            return []

        try:
            for d in hidapi_module.enumerate():
                if vid is not None and d['vendor_id'] != vid:
                    continue
                if pid is not None and d['product_id'] != pid:
                    continue
                path = d['path']
                if isinstance(path, bytes):
                    path = path.decode('latin-1', errors='replace')
                self.devices.append((
                    d['vendor_id'], d['product_id'],
                    d.get('serial_number', '') or '',
                    path
                ))
        except Exception:
            pass
        return self.devices

    def select(self, index: int) -> bool:
        if index < 0 or index >= len(self.devices):
            return False
        vid, pid, serial, path = self.devices[index]
        try:
            if isinstance(path, str):
                path = path.encode('latin-1')
            self.device = hidapi_module.device()
            self.device.open_path(path)
            self.selected_index = index
            return True
        except Exception as e:
            print(f"[ERROR] Cannot open HID device #{index}: {e}", file=sys.stderr)
            return False

    def _build_report(self, i2c_addr: int, op: int, reg: int, length: int, data: bytes = b'') -> bytes:
        """Build HID OUT report: [ReportID, I2Caddr, Op, Reg, Len, Data...]"""
        report = bytearray([HID_REPORT_ID, i2c_addr & 0x7F, op, reg, length])
        if data:
            report.extend(data)
        # Pad to 64 bytes
        report.extend(b'\x00' * (64 - len(report)))
        return bytes(report[:64])

    def _parse_in_report(self, data: bytes) -> Tuple[int, int, bytes]:
        """Parse HID IN report: [ReportID, Status, Length, Data...]"""
        if len(data) < 3:
            return -1, 0, b''
        return data[1], data[2], data[3:]

    def i2c_read(self, i2c_addr: int, reg: int, length: int = 1) -> bytes:
        if not self.device:
            raise RuntimeError("No HID device selected")
        out_report = self._build_report(i2c_addr, HID_OP_READ, reg, length)
        self.device.write(out_report)
        in_data = self.device.read(64, timeout_ms=3000)
        if not in_data:
            raise IOError("HID read timeout")
        status, length, data = self._parse_in_report(bytes(in_data))
        if status != 0:
            raise IOError(f"HID I2C NAK (status={status})")
        return data[:length]

    def i2c_write(self, i2c_addr: int, reg: int, data: bytes) -> int:
        if not self.device:
            raise RuntimeError("No HID device selected")
        out_report = self._build_report(i2c_addr, HID_OP_WRITE, reg, len(data), data)
        self.device.write(out_report)
        in_data = self.device.read(64, timeout_ms=3000)
        if not in_data:
            raise IOError("HID write timeout")
        status, length, _ = self._parse_in_report(bytes(in_data))
        return length if status == 0 else -1

    def reg_read(self, reg: int) -> int:
        if not self.device:
            raise RuntimeError("No HID device selected")
        out_report = self._build_report(0, 0x10, reg, 1)
        self.device.write(out_report)
        in_data = self.device.read(64, timeout_ms=3000)
        if not in_data:
            raise IOError("HID reg read timeout")
        _, _, data = self._parse_in_report(bytes(in_data))
        return data[0] if data else -1

    def reg_write(self, reg: int, value: int) -> bool:
        if not self.device:
            raise RuntimeError("No HID device selected")
        out_report = self._build_report(0, 0x11, reg, 1, bytes([value & 0xFF]))
        self.device.write(out_report)
        return True


# ============================================================================
# Hex Dump Utility
# ============================================================================

def hex_dump(addr: int, data: bytes, bytes_per_line: int = 16) -> List[str]:
    lines = []
    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        hex_part = ' '.join(f'{b:02X}' for b in chunk)
        ascii_part = ''.join(chr(b) if 32 <= b < 127 else '.' for b in chunk)
        lines.append(f"  {addr + i:08X}: {hex_part:<{bytes_per_line * 3}} {ascii_part}")
    return lines


# ============================================================================
# CLI Commands - HID
# ============================================================================

def cmd_device_list(hid: HIDDebugChannel, args):
    """List all connected HID devices"""
    devices = hid.enumerate()
    if not devices:
        print("[INFO] No HID devices found")
        return 0
    print(f"[INFO] Found {len(devices)} HID device(s)")
    for i, (vid, pid, serial, path) in enumerate(devices):
        print(f"  #{i}: VID=0x{vid:04X} PID=0x{pid:04X} Serial={serial}")
        print(f"       Path: {path[:60]}...")
    return 0


def cmd_device_select(hid: HIDDebugChannel, args):
    """Select a HID device by index"""
    devices = hid.enumerate()
    if not devices:
        print("[ERROR] No HID devices found", file=sys.stderr)
        return 2
    idx = int(args.index)
    if not hid.select(idx):
        print(f"[ERROR] Invalid device index: {idx}", file=sys.stderr)
        return 1
    vid, pid, serial, _ = devices[idx]
    print(f"[INFO] Selected device #{idx}: VID=0x{vid:04X} PID=0x{pid:04X}")
    return 0


def cmd_hid_read(hid: HIDDebugChannel, args):
    """I2C read via HID bridge"""
    if not HAS_HIDAPI:
        print("[ERROR] hidapi not available. Install with: pip install hidapi", file=sys.stderr)
        return 1
    i2c_addr = int(args.i2c_addr, 0) & 0x7F
    reg = int(args.reg, 0) & 0xFF
    length = int(args.length) if args.length else 1
    try:
        data = hid.i2c_read(i2c_addr, reg, length)
        hex_lines = hex_dump((i2c_addr << 8) | reg, data)
        for line in hex_lines:
            print(line)
        return 0
    except Exception as e:
        print(f"[ERROR] I2C read failed: {e}", file=sys.stderr)
        return 4


def cmd_hid_write(hid: HIDDebugChannel, args):
    """I2C write via HID bridge"""
    if not HAS_HIDAPI:
        print("[ERROR] hidapi not available. Install with: pip install hidapi", file=sys.stderr)
        return 1
    i2c_addr = int(args.i2c_addr, 0) & 0x7F
    reg = int(args.reg, 0) & 0xFF
    data = bytes(int(x, 0) & 0xFF for x in args.data)
    try:
        written = hid.i2c_write(i2c_addr, reg, data)
        print(f"[OK] {written} bytes written")
        return 0
    except Exception as e:
        print(f"[ERROR] I2C write failed: {e}", file=sys.stderr)
        return 4


def cmd_reg_read(hid: HIDDebugChannel, args):
    """Read HID bridge register"""
    if not HAS_HIDAPI:
        print("[ERROR] hidapi not available", file=sys.stderr)
        return 1
    reg = int(args.reg, 0) & 0xFF
    try:
        val = hid.reg_read(reg)
        print(f"[DATA] {val:02X}")
        return 0
    except Exception as e:
        print(f"[ERROR] Register read failed: {e}", file=sys.stderr)
        return 1


def cmd_reg_write(hid: HIDDebugChannel, args):
    """Write HID bridge register"""
    if not HAS_HIDAPI:
        print("[ERROR] hidapi not available", file=sys.stderr)
        return 1
    reg = int(args.reg, 0) & 0xFF
    val = int(args.data, 0) & 0xFF
    try:
        hid.reg_write(reg, val)
        print(f"[OK] Register {reg:02X} = {val:02X}")
        return 0
    except Exception as e:
        print(f"[ERROR] Register write failed: {e}", file=sys.stderr)
        return 1


def cmd_monitor(hid: HIDDebugChannel, args):
    """Monitor HID reports (hotplug detection)"""
    if not HAS_HIDAPI:
        print("[ERROR] hidapi not available", file=sys.stderr)
        return 1
    interval_ms = int(args.interval) if args.interval else 100
    print(f"[INFO] Monitoring HID reports every {interval_ms}ms (Ctrl+C to stop)...")

    last_count = len(hid.devices)
    running = True

    def poll():
        nonlocal last_count, running
        while running:
            devs = hid.enumerate()
            if len(devs) != last_count:
                print(f"[INFO] Device {'connected' if len(devs) > last_count else 'disconnected'}")
                last_count = len(devs)
            time.sleep(interval_ms / 1000.0)

    t = threading.Thread(target=poll, daemon=True)
    t.start()
    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        running = False
        print("\n[INFO] Stopped.")
    return 0


# ============================================================================
# CLI Commands - MSC
# ============================================================================

def cmd_msc_list(args):
    """List removable MSC devices"""
    drives = find_msc_devices()
    if not drives:
        print("[INFO] No removable MSC devices found")
        return 1
    print(f"[INFO] Found {len(drives)} removable drive(s):")
    for d in drives:
        print(f"  {d}")
    return 0


def cmd_msc_select(args, ctx: dict):
    """Select MSC device by drive letter"""
    drive = args.drive.upper()
    if len(drive) == 1:
        drive += ":"
    try:
        msc = MSCDebugChannel(drive)
        ctx['msc'] = msc
        print(f"[INFO] Selected MSC device: {drive}")
        return 0
    except OSError as e:
        print(f"[ERROR] Cannot open {drive}: {e}", file=sys.stderr)
        return 1


def cmd_msc_info(msc: MSCDebugChannel, args):
    """Read MSC device info"""
    try:
        info = msc.get_info()
        fw_major = (info.u16FwVersion >> 8) & 0xFF
        fw_minor = info.u16FwVersion & 0xFF
        build_y = (info.u16BuildDate >> 12) & 0xF
        build_m = (info.u16BuildDate >> 8) & 0xF
        build_d = info.u16BuildDate & 0x1F
        print("=== M487 Device Info ===")
        print(f"  Chip ID     : 0x{info.u32ChipId:08X}")
        print(f"  FW Version  : {fw_major}.{fw_minor:02d} (0x{info.u16FwVersion:04X})")
        print(f"  Build Date  : 20{build_y:02d}-{build_m:02d}-{build_d:02d}")
        print(f"  Flash Size  : {info.u32FlashSize // 1024} KB")
        print(f"  SRAM Size   : {info.u32RamSize // 1024} KB")
        print(f"  I2C Speed   : {info.u8I2cSpeed} kHz")
        print(f"  I2C Addr    : 0x{info.u8I2cAddr:02X}")
        print(f"  Log Head    : {info.u32LogHead}")
        print(f"  Log Tail    : {info.u32LogTail}")
        print(f"  Log Count   : {info.u32LogCount}")
        return 0
    except Exception as e:
        print(f"[ERROR] MSC info failed: {e}", file=sys.stderr)
        return 1


def cmd_msc_readmem(msc: MSCDebugChannel, args):
    """Read memory via MSC debug channel"""
    addr = int(args.addr, 0)
    length = int(args.length, 0)
    if length == 0:
        length = 16
    try:
        data = msc.read_mem(addr, length)
        print(f"Memory @ 0x{addr:08X} ({len(data)} bytes):")
        for line in hex_dump(addr, data):
            print(line)
        return 0
    except Exception as e:
        print(f"[ERROR] Read memory failed: {e}", file=sys.stderr)
        return 1


def cmd_msc_writemem(msc: MSCDebugChannel, args):
    """Write memory via MSC debug channel"""
    addr = int(args.addr, 0)
    hex_str = args.hex_data
    # Parse hex string (support 0x prefix)
    if hex_str.startswith('0x') or hex_str.startswith('0X'):
        hex_str = hex_str[2:]
    data = bytes.fromhex(hex_str)
    try:
        written = msc.write_mem(addr, data)
        print(f"Write memory @ 0x{addr:08X}: {written} bytes written")
        return 0
    except Exception as e:
        print(f"[ERROR] Write memory failed: {e}", file=sys.stderr)
        return 1


def cmd_msc_readreg(msc: MSCDebugChannel, args):
    """Read CPU register via MSC debug channel"""
    reg_id = int(args.reg_id, 0)
    reg_names = [
        "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10",
        "R11", "R12", "SP", "LR", "PC", "xPSR", "MSP", "PSP",
        "???", "PRIMASK", "CONTROL", "BASEPRI"
    ]
    try:
        val = msc.read_reg(reg_id)
        reg_name = reg_names[reg_id] if reg_id < len(reg_names) else "???"
        print(f"{reg_name} (reg {reg_id}) = 0x{val:08X}")
        return 0
    except Exception as e:
        print(f"[ERROR] Read register failed: {e}", file=sys.stderr)
        return 1


def cmd_msc_writereg(msc: MSCDebugChannel, args):
    """Write CPU register via MSC debug channel"""
    reg_id = int(args.reg_id, 0)
    value = int(args.value, 0)
    reg_names = [
        "R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7", "R8", "R9", "R10",
        "R11", "R12", "SP", "LR", "PC", "xPSR", "MSP", "PSP",
        "???", "PRIMASK", "CONTROL", "BASEPRI"
    ]
    try:
        ok = msc.write_reg(reg_id, value)
        reg_name = reg_names[reg_id] if reg_id < len(reg_names) else "???"
        print(f"Write {reg_name} (reg {reg_id}) = 0x{value:08X} -> {'OK' if ok else 'FAILED'}")
        return 0 if ok else 1
    except Exception as e:
        print(f"[ERROR] Write register failed: {e}", file=sys.stderr)
        return 1


def cmd_msc_log(msc: MSCDebugChannel, args):
    """Read debug log via MSC debug channel"""
    count = int(args.count) if args.count else 64
    level_names = ["ERR", "WARN", "INFO", "DBG", "TRC"]
    module_names = ["MAIN", "USB", "MSC", "I2C", "HID"]
    try:
        entries = msc.read_log(count)
        print(f"=== Debug Log ({len(entries)} entries) ===")
        for e in entries:
            level = level_names[e.u8Level] if e.u8Level < 5 else "???"
            module = module_names[e.u8Module] if e.u8Module < 5 else "???"
            data_str = ' '.join(f'{b:02X}' for b in e.au8Data[:e.u8Len]) if e.u8Len > 0 else ''
            print(f"  [{level}][{module}][0x{e.u8Code:02X}] TS=0x{e.u32Timestamp:08X} {data_str}")
        return 0
    except Exception as e:
        print(f"[ERROR] Read log failed: {e}", file=sys.stderr)
        return 1


def cmd_msc_echo(msc: MSCDebugChannel, args):
    """Echo test via MSC debug channel"""
    try:
        result = msc.echo(args.text)
        print(f"Echo back: {result}")
        return 0
    except Exception as e:
        print(f"[ERROR] Echo failed: {e}", file=sys.stderr)
        return 1


# ============================================================================
# MSC Auto-detect Context Manager
# ============================================================================

def auto_open_msc(ctx: dict):
    """Auto-detect and open MSC device if not already open"""
    if 'msc' in ctx and ctx['msc'] is not None:
        return ctx['msc']
    drives = find_msc_devices()
    if not drives:
        raise OSError("No MSC device found. Is M487 connected?")
    msc = MSCDebugChannel(drives[0])
    ctx['msc'] = msc
    print(f"[INFO] Auto-detected MSC device: {drives[0]}", file=sys.stderr)
    return msc


# ============================================================================
# Main Parser & Entry Point
# ============================================================================

def build_parser():
    parser = argparse.ArgumentParser(
        prog='hidtool',
        description='Unified HID + MSC Debug Channel CLI Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument('-v', '--verbose', action='store_true', help='Verbose output')
    parser.add_argument('-q', '--quiet', action='store_true', help='Quiet mode')
    parser.add_argument('-V', '--version', action='version', version='hidtool v1.0.0')

    sub = parser.add_subparsers(dest='command', help='Command to execute')

    # --- Device subcommands ---
    dev_parser = sub.add_parser('device', help='HID device management')
    dev_sub = dev_parser.add_subparsers(dest='device_cmd')

    dlist = dev_sub.add_parser('list', help='List HID devices')
    dlist.set_defaults(func=cmd_device_list)

    dsel = dev_sub.add_parser('select', help='Select HID device')
    dsel.add_argument('index', help='Device index')
    dsel.set_defaults(func=cmd_device_select)

    # --- I2C read/write ---
    read_p = sub.add_parser('read', help='I2C read via HID bridge')
    read_p.add_argument('i2c_addr', help='I2C address (7-bit, e.g. 0x50)')
    read_p.add_argument('reg', help='Register/offset address')
    read_p.add_argument('length', nargs='?', help='Number of bytes to read')
    read_p.set_defaults(func=cmd_hid_read)

    write_p = sub.add_parser('write', help='I2C write via HID bridge')
    write_p.add_argument('i2c_addr', help='I2C address (7-bit)')
    write_p.add_argument('reg', help='Register/offset address')
    write_p.add_argument('data', nargs='+', help='Byte data to write')
    write_p.set_defaults(func=cmd_hid_write)

    # --- Register access ---
    reg_p = sub.add_parser('reg', help='HID bridge register access')
    reg_sub = reg_p.add_subparsers(dest='reg_cmd')

    rr_p = reg_sub.add_parser('read', help='Read register')
    rr_p.add_argument('reg', help='Register address')
    rr_p.set_defaults(func=cmd_reg_read)

    rw_p = reg_sub.add_parser('write', help='Write register')
    rw_p.add_argument('reg', help='Register address')
    rw_p.add_argument('data', help='Data byte')
    rw_p.set_defaults(func=cmd_reg_write)

    # --- HID monitor ---
    mon_p = sub.add_parser('monitor', help='Monitor HID reports')
    mon_p.add_argument('interval', nargs='?', help='Polling interval in ms')
    mon_p.set_defaults(func=cmd_monitor)

    # --- MSC subcommands ---
    msc_p = sub.add_parser('msc', help='MSC Debug Channel commands')
    msc_sub = msc_p.add_subparsers(dest='msc_cmd')

    msc_list_p = msc_sub.add_parser('list', help='List removable MSC drives')
    msc_list_p.set_defaults(func=lambda a, ctx: cmd_msc_list(a))

    msc_sel_p = msc_sub.add_parser('select', help='Select MSC device by drive letter')
    msc_sel_p.add_argument('drive', help='Drive letter (e.g. E:)')
    msc_sel_p.set_defaults(func=lambda a, ctx: cmd_msc_select(a, ctx))

    msc_info_p = msc_sub.add_parser('info', help='Read MSC device info')
    msc_info_p.set_defaults(func=lambda a, ctx: cmd_msc_info(auto_open_msc(ctx), a))

    msc_rm_p = msc_sub.add_parser('readmem', help='Read memory via MSC')
    msc_rm_p.add_argument('addr', help='Memory address (e.g. 0x20000000)')
    msc_rm_p.add_argument('length', help='Number of bytes')
    msc_rm_p.set_defaults(func=lambda a, ctx: cmd_msc_readmem(auto_open_msc(ctx), a))

    msc_wm_p = msc_sub.add_parser('writemem', help='Write memory via MSC')
    msc_wm_p.add_argument('addr', help='Memory address')
    msc_wm_p.add_argument('hex_data', help='Hex data string')
    msc_wm_p.set_defaults(func=lambda a, ctx: cmd_msc_writemem(auto_open_msc(ctx), a))

    msc_rr_p = msc_sub.add_parser('readreg', help='Read CPU register')
    msc_rr_p.add_argument('reg_id', help='Register ID (0-22)')
    msc_rr_p.set_defaults(func=lambda a, ctx: cmd_msc_readreg(auto_open_msc(ctx), a))

    msc_wr_p = msc_sub.add_parser('writereg', help='Write CPU register')
    msc_wr_p.add_argument('reg_id', help='Register ID (0-22)')
    msc_wr_p.add_argument('value', help='Value (e.g. 0x12345678)')
    msc_wr_p.set_defaults(func=lambda a, ctx: cmd_msc_writereg(auto_open_msc(ctx), a))

    msc_log_p = msc_sub.add_parser('log', help='Read debug log')
    msc_log_p.add_argument('count', nargs='?', help='Number of entries (default=64)')
    msc_log_p.set_defaults(func=lambda a, ctx: cmd_msc_log(auto_open_msc(ctx), a))

    msc_echo_p = msc_sub.add_parser('echo', help='Echo test')
    msc_echo_p.add_argument('text', help='Text to echo')
    msc_echo_p.set_defaults(func=lambda a, ctx: cmd_msc_echo(auto_open_msc(ctx), a))

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 0

    ctx = {}  # Shared context for MSC/HID state

    # Initialize HID channel
    hid = HIDDebugChannel()

    # Route to appropriate handler
    try:
        if hasattr(args, 'func'):
            # Standard commands
            if args.command in ('device',):
                # Device commands don't need MSC
                result = args.func(hid, args)
            elif args.command == 'msc':
                # MSC commands use context
                result = args.func(args, ctx)
            else:
                # Other HID commands
                result = args.func(hid, args)
        else:
            parser.print_help()
            return 0
    except KeyboardInterrupt:
        print("\n[INFO] Interrupted.")
        return 130
    except Exception as e:
        if args.verbose if hasattr(args, 'verbose') else False:
            import traceback
            traceback.print_exc()
        print(f"[ERROR] {e}", file=sys.stderr)
        return 99
    finally:
        # Cleanup
        if 'msc' in ctx and ctx['msc'] is not None:
            ctx['msc'].close()

    return result if result is not None else 0


if __name__ == '__main__':
    sys.exit(main())
