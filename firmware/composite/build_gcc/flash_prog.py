# M487 APROM Flash Programmer via ICE/GDB
# Strategy:
# 1. Load firmware to SRAM (0x20000000)
# 2. Write flash programming stub to SRAM (0x20028000)
# 3. Execute stub to erase APROM and program from SRAM data
# 4. Verify

import gdb
import struct

FLASH_BASE = 0x10000000   # APROM physical base
SRAM_BASE  = 0x20000000   # SRAM base
SRAM_END   = 0x20028000   # SRAM end (160KB)
FMC_BASE   = 0x400C0000   # FMC base

# FMC registers
ISPCTL  = FMC_BASE + 0x00
ISPCMD  = FMC_BASE + 0x04
ISPADDR = FMC_BASE + 0x08
ISPDAT  = FMC_BASE + 0x0C
ISPTRIG = FMC_BASE + 0x10

# FMC commands
CMD_ISP      = 0x01
CMD_READ     = 0x00
CMD_PROGRAM  = 0x21
CMD_ERASE    = 0x22

PAGE_SIZE    = 2048        # APROM page size
FIRMWARE_LEN = 64088       # firmware size

def read_mem(addr, size=4):
    val = gdb.parse_and_eval(f'*(unsigned int*){addr}')
    return int(val)

def write_mem(addr, val):
    gdb.execute(f'set *((unsigned int*){addr}) = {val}')

def fmc_cmd(cmd, addr, data):
    """Execute FMC command"""
    gdb.execute(f'set *((unsigned int*){ISPCTL}) = 0x01')  # ISPEN
    gdb.execute(f'set *((unsigned int*){ISPCMD}) = {cmd}')
    gdb.execute(f'set *((unsigned int*){ISPADDR}) = {addr}')
    gdb.execute(f'set *((unsigned int*){ISPDAT}) = {data}')
    gdb.execute(f'set *((unsigned int*){ISPTRIG}) = 0x01')
    gdb.execute('set $x = *((unsigned int*)0x400C0200)')  # CRSR
    gdb.execute('set $x = *((unsigned int*)0x400C0200)')

def erase_aprom():
    print(f"Erasing APROM {FLASH_BASE}...")
    num_pages = (FIRMWARE_LEN + PAGE_SIZE - 1) // PAGE_SIZE
    for i in range(num_pages):
        page_addr = FLASH_BASE + i * PAGE_SIZE
        print(f"  Erase page {i} @ 0x{page_addr:08X}")
        fmc_cmd(CMD_ERASE, page_addr, 0)
    print("Erase complete")

def program_aprom():
    print(f"Programming APROM from SRAM to flash...")
    num_words = (FIRMWARE_LEN + 3) // 4
    for i in range(num_words):
        flash_addr = FLASH_BASE + i * 4
        sram_addr  = SRAM_BASE + i * 4
        val = read_mem(sram_addr)
        fmc_cmd(CMD_PROGRAM, flash_addr, val)
        if i % 500 == 0:
            print(f"  Progress: {i}/{num_words} words")
    print("Program complete")

def verify():
    print("Verifying...")
    errors = 0
    for i in range(FIRMWARE_LEN // 4):
        flash_addr = FLASH_BASE + i * 4
        sram_addr  = SRAM_BASE + i * 4
        fv = read_mem(flash_addr)
        sv = read_mem(sram_addr)
        if fv != sv:
            errors += 1
            if errors <= 5:
                print(f"  Mismatch @ {i}: flash=0x{fv:08X} sram=0x{sv:08X}")
    if errors == 0:
        print("Verification PASSED")
    else:
        print(f"Verification FAILED: {errors} errors")

# Main sequence
gdb.execute(f'target remote localhost:3333')
gdb.execute('monitor halt')

# Load firmware to SRAM
print("Loading firmware to SRAM...")
gdb.execute(f'load firmware.elf {SRAM_BASE}')

# Verify SRAM loaded
sv = read_mem(SRAM_BASE)
print(f"SRAM @ {SRAM_BASE}: 0x{sv:08X}")

# Erase APROM
erase_aprom()

# Program APROM
program_aprom()

# Verify
verify()

gdb.execute('detach')
print("Done")
