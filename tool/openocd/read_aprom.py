import socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('localhost', 4444))
s.settimeout(2.0)

s.send(b'mdw 0x10000000 32\n')
time.sleep(0.5)
try:
    data = s.recv(4096)
    print(data.decode('ascii', errors='replace'))
except:
    pass

s.send(b'quit\n')
s.close()
