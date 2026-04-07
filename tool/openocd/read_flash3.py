import socket, time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(3.0)
s.connect(('localhost', 4444))
log_file = open(r'D:\AiWorkSpace\M487_ScsiTool\tool\openocd\flash_read_log3.txt', 'w', encoding='utf-8', errors='replace')

def get_all():
    data = b''
    for _ in range(50):
        try:
            chunk = s.recv(8192)
            if not chunk:
                break
            data += chunk
        except:
            break
    return data.decode('utf-8', errors='replace')

def log(msg):
    log_file.write(msg + '\n')
    log_file.flush()

# Reset and halt
s.send(b'reset halt\n')
time.sleep(2.0)
log("=== RESET HALT ===")
log(get_all())

# Check registers immediately
s.send(b'reg pc\n')
time.sleep(0.3)
log("=== PC ===")
log(get_all())

s.send(b'reg msp\n')
time.sleep(0.3)
log("=== MSP ===")
log(get_all())

# Read flash at 0x10000000
s.send(b'mdw 0x10000000 8\n')
time.sleep(0.5)
log("=== FLASH 0x10000000 ===")
log(get_all())

# Read flash vector table at 0x00000000 (alias)
s.send(b'mdw 0x00000000 8\n')
time.sleep(0.5)
log("=== FLASH alias 0x00000000 ===")
log(get_all())

# Read SRAM
s.send(b'mdw 0x20000000 8\n')
time.sleep(0.5)
log("=== SRAM 0x20000000 ===")
log(get_all())

# CHIP_ID
s.send(b'mdw 0x50000000 4\n')
time.sleep(0.5)
log("=== CHIP_ID ===")
log(get_all())

log_file.close()
print("DONE")
