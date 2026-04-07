import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(3.0)
s.connect(('localhost', 4444))
sf = s.makefile('r', buffering=1)

# Reset and let MCU run briefly
s.send(b'reset run\n')
time.sleep(1.5)
s.send(b'halt\n')
time.sleep(0.5)
s.send(b'reg pc\n')
time.sleep(0.3)

# Read response
data = b''
while True:
    try:
        chunk = s.recv(4096)
        if not chunk:
            break
        data += chunk
    except:
        break

print(data.decode('ascii', errors='replace'))
s.close()
