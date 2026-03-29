import hid

# List all HID devices
devices = hid.enumerate()
print('HID devices:', len(devices))
i = 0
for d in devices:
    i += 1
    vid = d.get('vendor_id', 0)
    pid = d.get('product_id', 0)
    path = d.get('path', '?')
    print(f'  [{i}] VID={vid:04X} PID={pid:04X} path={path}')
    if vid == 0x0416 and pid == 0x511C:
        print('    ^^ NU-LINK FOUND ^^')
        try:
            h = hid.device(path=path)
            print(f'  Opened!')
            try:
                mfg = h.get_manufacturer_string()
                prod = h.get_product_string()
                serial = h.get_serial_number_string()
                print(f'  Manufacturer: {mfg}')
                print(f'  Product: {prod}')
                print(f'  Serial: {serial}')
            except:
                pass
            # Read something
            try:
                data = h.read(64, timeout_ms=1000)
                print(f'  Read: {list(data)}')
            except Exception as e:
                print(f'  Read error: {e}')
            h.close()
        except Exception as e:
            print(f'  Open error: {e}')
