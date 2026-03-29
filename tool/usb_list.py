import usb.core

# Find all USB devices with verbose output
devs = usb.core.find(find_all=True)
devs_list = list(devs)
print('Total USB devices:', len(devs_list))

for d in devs_list:
    vid = d.idVendor
    pid = d.idProduct
    try:
        mfg = usb.util.get_string(d, d.iManufacturer) if d.iManufacturer else ''
        prod = usb.util.get_string(d, d.iProduct) if d.iProduct else ''
    except:
        mfg = ''
        prod = ''
    print(f'  {vid:04X}:{pid:04X}  {mfg} / {prod}')
    if vid == 0x0416:
        print('    ^^ NUVOTON FOUND ^^')
        # Try to access it
        try:
            cfg = d.get_active_configuration()
            print(f'    Config: {cfg.bConfigurationValue}')
            for iface in cfg:
                print(f'    Interface: {iface.bInterfaceNumber} Class={iface.bInterfaceClass}')
        except Exception as e:
            print(f'    Config error: {e}')
