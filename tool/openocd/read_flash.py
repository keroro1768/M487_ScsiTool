import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(3.0)
s.connect(('localhost', 4444))

def send_cmd(cmd):
    s.send((cmd + '\n').encode())
    import time
    time.sleep(0.3)
    data = b''
    while True:
        try:
            chunk = s.recv(4096)
            if not chunk:
                break
            data += chunk
        except:
            break
    return data.decode('ascii', errors='replace')

# Read APROM at 0x10000000 (first 32 words = 128 bytes)
print("=== APROM @ 0x10000000 (first 32 words) ===")
print(send_cmd('mdw 0x10000000 32'))

# Also read at 0x00000000
print("=== APROM alias @ 0x00000000 (first 32 words) ===")
print(send_cmd('mdw 0x00000000 32'))

# Read CHIP_ID
print("=== CHIP_ID @ 0x50000000 ===")
print(send_cmd('mdw 0x50000000 4'))

s.send(b'quit\n')
s.close()
