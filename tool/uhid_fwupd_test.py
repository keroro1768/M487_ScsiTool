#!/usr/bin/env python3
"""
Virtual HID Device via uhid for FWUPD Testing
- Input Report:  Report ID 0x02, 65 bytes
- Output Report: Report ID 0x03, 33 bytes
"""

import os
import struct
import threading
import time

UHID_MAX_WRITE_SIZE = 64  # Linux uhid max write size

# uhid ioctl numbers (from kernel)
UHID_CREATE = 0x06
UHID_DESTROY = 0x07
UHID_START = 0x08
UHID_STOP = 0x09

# HID Report Descriptor for:
#   Input Report  (ID 0x02): 65 bytes
#   Output Report (ID 0x03): 33 bytes
# Build it using HID descriptor utility or manual bytes

def build_report_descriptor():
    """Build HID Report Descriptor for FWUPD virtual device."""
    # Report ID 0x02 = Input, 64 bytes data + 1 byte ID = 65 bytes
    # Report ID 0x03 = Output, 32 bytes data + 1 byte ID = 33 bytes
    
    rdesc = bytes([
        # Report ID 0x02 (Input, 64 bytes data + 1 byte ID = 65 bytes total)
        0x85, 0x02,        # Report ID (2)
        0x95, 0x40,        # Report Count (64)
        0x75, 0x08,        # Report Size (8)
        0x81, 0x02,        # Input (Data, Variable, Absolute)
        
        # Report ID 0x03 (Output, 32 bytes data + 1 byte ID = 33 bytes total)
        0x85, 0x03,        # Report ID (3)
        0x95, 0x20,        # Report Count (32)
        0x75, 0x08,        # Report Size (8)
        0x91, 0x02,        # Output (Data, Variable, Absolute)
        
        # Feature report (same as input)
        0x85, 0x02,        # Report ID (2)
        0x95, 0x40,        # Report Count (64)
        0x75, 0x08,        # Report Size (8)
        0xB1, 0x02,        # Feature (Data, Variable, Absolute)
    ])
    return rdesc


def pack_uhid_create(rdesc, name="fwupd-virtual-hid"):
    """Pack UHID_CREATE ioctl data."""
    # struct uhid_create_req {
    #   __u8 name[128];
    #   __u8 phys[64];
    #   __u8 uniq[64];
    #   __u8 rd_size;
    #   __u8 rd[UHID_MAX_WRITE_SIZE]; // 64 bytes max
    #   __u16 bus;
    #   __u32 vendor;
    #   __u32 product;
    #   __u32 version;
    #   __u32 country;
    # }
    name_bytes = name.encode('utf-8')[:127].ljust(128, b'\x00')
    phys_bytes = b'\x00' * 64
    uniq_bytes = b'\x00' * 64
    rdesc_size = len(rdesc)
    rdesc_padded = rdesc.ljust(UHID_MAX_WRITE_SIZE, b'\x00')
    
    return struct.pack('=128s64s64sBBB',
                       name_bytes, phys_bytes, uniq_bytes,
                       rdesc_size, rdesc_size, 0) + rdesc_padded[:UHID_MAX_WRITE_SIZE]


def pack_uhid_start():
    """Pack UHID_START ioctl - no special data needed."""
    return bytes()


def main():
    import argparse
    parser = argparse.ArgumentParser(description='Virtual HID device via uhid')
    parser.add_argument('--name', default='fwupd-virtual-hid', help='Device name')
    parser.add_argument('--bus', type=int, default=0, help='Bus type (0=USB)')
    parser.add_argument('--vendor', type=int, default=0x0000, help='Vendor ID')
    parser.add_argument('--product', type=int, default=0x0000, help='Product ID')
    args = parser.parse_args()
    
    # Open uhid device
    uhid_fd = os.open('/dev/uhid', os.O_RDWR)
    print(f'[uhid] Opened /dev/uhid (fd={uhid_fd})')
    
    # Build report descriptor
    rdesc = build_report_descriptor()
    print(f'[uhid] Report descriptor: {len(rdesc)} bytes')
    
    # Create device
    create_data = pack_uhid_create(rdesc, args.name)
    os.write(uhid_fd, create_data)
    print('[uhid] Device created')
    
    # Event thread to read from uhid
    stop_event = threading.Event()
    
    def reader():
        print('[uhid] Reader thread started')
        while not stop_event.is_set():
            try:
                import select
                r, _, _ = select.select([uhid_fd], [], [], 0.5)
                if r:
                    data = os.read(uhid_fd, 1024)
                    if len(data) >= 2:
                        op = data[0]
                        if op == 0x01:  # UHID_INPUT
                            rnum = data[1]
                            rlen = len(data) - 2
                            print(f'[uhid] INPUT  report: ID=0x{rnum:02X} len={rlen}')
                            # Echo back as test
                            # You can implement fwupd response here
                        elif op == 0x02:  # UHID_OUTPUT
                            rnum = data[1]
                            rlen = len(data) - 2
                            print(f'[uhid] OUTPUT report: ID=0x{rnum:02X} len={rlen}')
                        elif op == 0x03:  # UHID_FEATURE
                            rnum = data[1]
                            rlen = len(data) - 2
                            print(f'[uhid] FEATURE report: ID=0x{rnum:02X} len={rlen}')
                        elif op == 0x04:  # UHID_START
                            print('[uhid] Device started (feature启用)')
                        elif op == 0x05:  # UHID_STOP
                            print('[uhid] Device stopped')
                            break
                        else:
                            print(f'[uhid] Unknown event: {data.hex()}')
            except Exception as e:
                if not stop_event.is_set():
                    print(f'[uhid] Read error: {e}')
                    time.sleep(0.1)
        print('[uhid] Reader thread exited')
    
    reader_thread = threading.Thread(target=reader)
    reader_thread.start()
    
    print('[uhid] Virtual HID device ready for FWUPD testing')
    print('Press Ctrl+C to exit')
    
    try:
        while True:
            time.sleep(1)
            # Optional: send periodic input reports to test host
            # report = bytes([0x02]) + b'\x00' * 64  # Report ID 0x02 + 64 bytes data
            # os.write(uhid_fd, report)
    except KeyboardInterrupt:
        print('\n[uhid] Shutting down...')
        stop_event.set()
        reader_thread.join(timeout=2)
        os.close(uhid_fd)
        print('[uhid] Done')


if __name__ == '__main__':
    main()
