import re
import sys

f = open(r'D:\\AiWorkSpace\\M487_ScsiTool\\tool\\openocd-src\\src\\flash\\nor\\numicro.c', 'r')
content = f.read()
f.close()

# Search for chip IDs or PDID patterns
lines = content.split('\n')
for i, line in enumerate(lines):
    if '0x1000' in line or 'PDID' in line or '0x4' in line.upper():
        print(f"{i+1}: {line}")
