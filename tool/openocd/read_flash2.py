import socket, time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(3.0)
s.connect(('localhost', 4444))
log_file = open(r'D:\AiWorkSpace\M487_ScsiTool\tool\openocd\flash_read_log.txt', 'w', encoding='utf-8')

def get_resp():
    data = b''
    for _ in range(20):
        try:
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
            if b'> ' in data or b'\r\n> ' in data:
                break
        except:
            break
    return data.decode('utf-8', errors='replace')

def log(msg):
    log_file.write(msg + '\n')
    log_file.flush()

s.send(b'reset halt\n')
time.sleep(1.5)
log(get_resp())

s.send(b'reg pc\n')
time.sleep(0.3)
log(get_resp())

s.send(b'reg msp\n')
time.sleep(0.3)
log(get_resp())

# Read APROM flash at 0x10000000 (first 8 words = vector table)
s.send(b'mdw 0x10000000 8\n')
time.sleep(0.3)
log(get_resp())

# Read SRAM at 0x20000000
s.send(b'mdw 0x20000000 8\n')
time.sleep(0.3)
log(get_resp())

# Read CHIP_ID
s.send(b'mdw 0x50000000 4\n')
time.sleep(0.3)
log(get_resp())

s.send(b'quit\n')
time.sleep(0.3)
s.close()
log_file.close()
print("DONE")
