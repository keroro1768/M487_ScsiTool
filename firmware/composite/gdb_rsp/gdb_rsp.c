/**
 * @file     gdb_rsp.c
 * @brief    GDB Remote Serial Protocol (RSP) Server Implementation
 * @version  1.0.0
 * 
 * Implements GDB RSP server for self-hosted debugging via MSC Debug Channel.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gdb_rsp.h"
#include "itm.h"

/*---------------------------------------------------------------------------------------------------------*/
/* Debug Register Access (ARM Cortex-M4)                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
static inline uint32_t read_xPSR(void) {
    uint32_t result;
    __asm volatile ("mrs %0, xPSR" : "=r"(result));
    return result;
}

static inline uint32_t read_pc(void) {
    uint32_t result;
    __asm volatile ("mov %0, pc" : "=r"(result));
    return result;
}

static inline uint32_t read_ctrl(void) {
    uint32_t result;
    __asm volatile ("mrs %0, CONTROL" : "=r"(result));
    return result;
}

static inline void write_pc(uint32_t value) {
    __asm volatile ("mov pc, %0" : : "r"(value));
}

static inline uint32_t read_msp(void) {
    uint32_t result;
    __asm volatile ("mrs %0, msp" : "=r"(result));
    return result;
}

static inline uint32_t read_psp(void) {
    uint32_t result;
    __asm volatile ("mrs %0, psp" : "=r"(result));
    return result;
}

/*---------------------------------------------------------------------------------------------------------*/
/* RSP State                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
static RSP_State_t s_RSPState = {
    .halted = false,
    .singleStep = false,
    .resumeAddr = 0,
    .breakpointAddr = 0
};

/*---------------------------------------------------------------------------------------------------------*/
/* Breakpoint Storage                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    bool active;
    BreakpointType type;
    uint32_t addr;
    uint8_t savedOpcode;  /* For software breakpoints */
} BreakpointEntry_t;

static BreakpointEntry_t s_breakpoints[RSP_MAX_BREAKPOINTS] = {0};

/*---------------------------------------------------------------------------------------------------------*/
/* Internal Function Prototypes                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
static uint8_t hex_nibble_to_hex(uint8_t nibble);
static uint8_t hex_to_nibble(uint8_t hex);
static void encode_hex(const uint8_t *pData, uint16_t len, char *pHex);
static int32_t decode_hex(const char *pHex, uint16_t hexLen, uint8_t *pData);
static int32_t parse_hex_value(const char *pHex, uint32_t *pValue);
static void format_hex_value(uint32_t value, int width, char *pHex);
static void send_response(const char *pMsg, char *pResponse, uint16_t maxLen);

/*---------------------------------------------------------------------------------------------------------*/
/* Register Layout (ARM Cortex-M4)                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define REG_COUNT  20  /* R0-R15, xPSR, MSP, PSP,PSR, PRIMASK, CONTROL, FAULTMASK, BASEPRI, BASEPRI_MAX, */

typedef struct {
    uint32_t r[16];     /* R0-R15 */
    uint32_t xPSR;
    uint32_t MSP;
    uint32_t PSP;
    uint32_t PRIMASK;
    uint32_t CONTROL;
    uint32_t FAULTMASK;
    uint32_t BASEPRI;
    uint32_t BASEPRI_MAX;
} RegisterFile_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Core Debug Access                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#define SCS_BASE      (0xE000E000)
#define DHCSR_BASE    (*(volatile uint32_t *)(SCS_BASE + 0xDF0))
#define DCSR_KEY_MASK 0xFFFF0000
#define DCSR_HALT     0x00000001
#define DCSR_STEP     0x00000002
#define DCSR_KEEP     0x00020000

static inline void debug_halt(void) {
    DHCSR_BASE = DCSR_KEY_MASK | DCSR_KEEP | DCSR_HALT;
}

static inline void debug_step(void) {
    DHCSR_BASE = DCSR_KEY_MASK | DCSR_KEEP | DCSR_STEP;
}

static inline void debug_resume(void) {
    DHCSR_BASE = DCSR_KEY_MASK | DCSR_KEEP;
}

static inline uint32_t debug_status(void) {
    return DHCSR_BASE;
}

/*---------------------------------------------------------------------------------------------------------*/
/* DWT Breakpoint Control                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#define DWT_BASE           (0xE0001000)
#define DWT_CTRL           (*(volatile uint32_t *)(DWT_BASE + 0x000))
#define DWT_COMP_BASE      (DWT_BASE + 0x020)
#define DWT_MASK_BASE      (DWT_BASE + 0x024)
#define DWT_FUNCTION_BASE  (DWT_BASE + 0x028)

#define DWT_FUNCTION_MATCH    0x00000001
#define DWT_FUNCTION_UBYTE   0x00000002

static inline void dwt_enable_breakpoint(int index, uint32_t addr) {
    if (index < 4) {  /* M487 has 4 DWT comparators */
        volatile uint32_t *comp = (volatile uint32_t *)(DWT_COMP_BASE + index * 12);
        volatile uint32_t *func = (volatile uint32_t *)(DWT_FUNCTION_BASE + index * 12);
        *comp = addr;
        *func = DWT_FUNCTION_MATCH;  /* Break on match */
    }
}

static inline void dwt_disable_breakpoint(int index) {
    if (index < 4) {
        volatile uint32_t *func = (volatile uint32_t *)(DWT_FUNCTION_BASE + index * 12);
        *func = 0;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Hex Conversion Utilities                                                                                */
/*---------------------------------------------------------------------------------------------------------*/

static uint8_t hex_to_nibble(uint8_t hex) {
    if (hex >= '0' && hex <= '9') return hex - '0';
    if (hex >= 'a' && hex <= 'f') return hex - 'a' + 10;
    if (hex >= 'A' && hex <= 'F') return hex - 'A' + 10;
    return 0;
}

static uint8_t hex_nibble_to_hex(uint8_t nibble) {
    nibble &= 0x0F;
    if (nibble < 10) return nibble + '0';
    return nibble - 10 + 'a';
}

static void encode_hex(const uint8_t *pData, uint16_t len, char *pHex) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        pHex[i * 2] = hex_nibble_to_hex(pData[i] >> 4);
        pHex[i * 2 + 1] = hex_nibble_to_hex(pData[i]);
    }
    pHex[len * 2] = '\0';
}

static int32_t decode_hex(const char *pHex, uint16_t hexLen, uint8_t *pData) {
    uint16_t i;
    if (hexLen % 2 != 0) return -1;
    
    for (i = 0; i < hexLen / 2; i++) {
        pData[i] = (hex_to_nibble(pHex[i * 2]) << 4) | hex_to_nibble(pHex[i * 2 + 1]);
    }
    return hexLen / 2;
}

static int32_t parse_hex_value(const char *pHex, uint32_t *pValue) {
    uint32_t value = 0;
    int digits = 0;
    
    while (*pHex) {
        if ((*pHex >= '0' && *pHex <= '9') ||
            (*pHex >= 'a' && *pHex <= 'f') ||
            (*pHex >= 'A' && *pHex <= 'F')) {
            value = (value << 4) | hex_to_nibble(*pHex);
            digits++;
        } else {
            break;
        }
        pHex++;
    }
    
    *pValue = value;
    return digits;
}

static void format_hex_value(uint32_t value, int width, char *pHex) {
    char tmp[11];
    int i, j;
    
    if (width == 0) width = 8;
    
    /* Convert to hex string (MSB first) */
    tmp[10] = '\0';
    for (i = 9, j = 0; i >= 0 && j < width; i--, j++) {
        tmp[i] = hex_nibble_to_hex(value >> (j * 4));
        value >>= 4;
    }
    
    /* Find first non-zero digit */
    for (i = 0; i < 10 && tmp[i] == '0'; i++);
    
    if (i == 10) {
        strcpy(pHex, "0");
    } else {
        strcpy(pHex, &tmp[i]);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Memory Access                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/

static uint8_t read_memory_u8(uint32_t addr) {
    return *(volatile uint8_t *)addr;
}

static void write_memory_u8(uint32_t addr, uint8_t value) {
    *(volatile uint8_t *)addr = value;
    __asm__ volatile ("isb" ::: "memory");
}

static uint32_t read_memory_u32(uint32_t addr) {
    return *(volatile uint32_t *)addr;
}

static void write_memory_u32(uint32_t addr, uint32_t value) {
    *(volatile uint32_t *)addr = value;
    __asm__ volatile ("isb" ::: "memory");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Register Read/Write                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/

static void read_registers(RegisterFile_t *pRegs) {
    uint32_t tmp;
    
    /* Read general purpose registers R0-R12 */
    __asm volatile ("mov %0, r0" : "=r"(tmp)); pRegs->r[0] = tmp;
    __asm volatile ("mov %0, r1" : "=r"(tmp)); pRegs->r[1] = tmp;
    __asm volatile ("mov %0, r2" : "=r"(tmp)); pRegs->r[2] = tmp;
    __asm volatile ("mov %0, r3" : "=r"(tmp)); pRegs->r[3] = tmp;
    __asm volatile ("mov %0, r4" : "=r"(tmp)); pRegs->r[4] = tmp;
    __asm volatile ("mov %0, r5" : "=r"(tmp)); pRegs->r[5] = tmp;
    __asm volatile ("mov %0, r6" : "=r"(tmp)); pRegs->r[6] = tmp;
    __asm volatile ("mov %0, r7" : "=r"(tmp)); pRegs->r[7] = tmp;
    __asm volatile ("mov %0, r8" : "=r"(tmp)); pRegs->r[8] = tmp;
    __asm volatile ("mov %0, r9" : "=r"(tmp)); pRegs->r[9] = tmp;
    __asm volatile ("mov %0, r10" : "=r"(tmp)); pRegs->r[10] = tmp;
    __asm volatile ("mov %0, r11" : "=r"(tmp)); pRegs->r[11] = tmp;
    __asm volatile ("mov %0, r12" : "=r"(tmp)); pRegs->r[12] = tmp;
    
    /* Read SP, LR, PC */
    pRegs->r[13] = read_msp();  /* MSP */
    pRegs->r[14] = 0;  /* LR - not easily readable */
    pRegs->r[15] = read_pc();  /* PC */
    
    /* Read xPSR */
    pRegs->xPSR = read_xPSR();
    
    /* Read special registers */
    pRegs->MSP = read_msp();
    pRegs->PSP = read_psp();
    
    __asm volatile ("mrs %0, PRIMASK" : "=r"(tmp)); pRegs->PRIMASK = tmp;
    __asm volatile ("mrs %0, CONTROL" : "=r"(tmp)); pRegs->CONTROL = tmp;
    __asm volatile ("mrs %0, FAULTMASK" : "=r"(tmp)); pRegs->FAULTMASK = tmp;
    __asm volatile ("mrs %0, BASEPRI" : "=r"(tmp)); pRegs->BASEPRI = tmp;
    __asm volatile ("mrs %0, BASEPRI_MAX" : "=r"(tmp)); pRegs->BASEPRI_MAX = tmp;
}

static void write_register(int regNum, uint32_t value) {
    switch (regNum) {
        case 0: __asm volatile ("mov r0, %0" : : "r"(value)); break;
        case 1: __asm volatile ("mov r1, %0" : : "r"(value)); break;
        case 2: __asm volatile ("mov r2, %0" : : "r"(value)); break;
        case 3: __asm volatile ("mov r3, %0" : : "r"(value)); break;
        case 4: __asm volatile ("mov r4, %0" : : "r"(value)); break;
        case 5: __asm volatile ("mov r5, %0" : : "r"(value)); break;
        case 6: __asm volatile ("mov r6, %0" : : "r"(value)); break;
        case 7: __asm volatile ("mov r7, %0" : : "r"(value)); break;
        case 8: __asm volatile ("mov r8, %0" : : "r"(value)); break;
        case 9: __asm volatile ("mov r9, %0" : : "r"(value)); break;
        case 10: __asm volatile ("mov r10, %0" : : "r"(value)); break;
        case 11: __asm volatile ("mov r11, %0" : : "r"(value)); break;
        case 12: __asm volatile ("mov r12, %0" : : "r"(value)); break;
        case 13: /* SP - use MSR */ __asm volatile ("msr msp, %0" : : "r"(value)); break;
        case 14: /* LR - not directly writable */ break;
        case 15: write_pc(value); break;
        default: break;
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* Breakpoint Management                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/

static int find_free_breakpoint_slot(void) {
    int i;
    for (i = 0; i < RSP_MAX_BREAKPOINTS; i++) {
        if (!s_breakpoints[i].active) {
            return i;
        }
    }
    return -1;
}

static int find_breakpoint_at(uint32_t addr) {
    int i;
    for (i = 0; i < RSP_MAX_BREAKPOINTS; i++) {
        if (s_breakpoints[i].active && s_breakpoints[i].addr == addr) {
            return i;
        }
    }
    return -1;
}

static int set_breakpoint(BreakpointType type, uint32_t addr) {
    int slot = find_free_breakpoint_slot();
    if (slot < 0) return -1;
    
    s_breakpoints[slot].active = true;
    s_breakpoints[slot].type = type;
    s_breakpoints[slot].addr = addr;
    
    if (type == BREAKPOINT_SW) {
        /* Save original opcode and replace with breakpoint */
        s_breakpoints[slot].savedOpcode = read_memory_u8(addr);
        write_memory_u8(addr, 0xBE);  /* BKPT instruction */
    } else if (type == BREAKPOINT_HW) {
        /* Use DWT for hardware breakpoint */
        dwt_enable_breakpoint(slot, addr);
    }
    
    return 0;
}

static int remove_breakpoint(BreakpointType type, uint32_t addr) {
    int slot = find_breakpoint_at(addr);
    if (slot < 0) return -1;
    
    if (s_breakpoints[slot].type == BREAKPOINT_SW) {
        /* Restore original opcode */
        write_memory_u8(addr, s_breakpoints[slot].savedOpcode);
    } else if (s_breakpoints[slot].type == BREAKPOINT_HW) {
        dwt_disable_breakpoint(slot);
    }
    
    s_breakpoints[slot].active = false;
    return 0;
}

/*---------------------------------------------------------------------------------------------------------*/
/* RSP Command Handlers                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/

static void handle_query_supported(char *pResponse, uint16_t maxLen) {
    /* Report supported features */
    const char *features = 
        "PacketSize=256;"        /* Maximum packet size */
        "qXfer:memory-map:read;" /* Memory map support */
        "QStartNoAckMode;"       /* No ACK mode */
        "QCatch;"                /* Catch reloads */
        "QNonBlocking;"          /* Non-blocking mode */
        "swbreak+;hwbreak+;"     /* Breakpoint support */
        "Endian:little;"         /* Endianness */
        ;
    strncpy(pResponse, features, maxLen - 1);
    pResponse[maxLen - 1] = '\0';
}

static void handle_query_attached(char *pResponse, uint16_t maxLen) {
    /* Return 1 = attached to existing process */
    strncpy(pResponse, "1", maxLen - 1);
}

static void handle_read_all_registers(char *pResponse, uint16_t maxLen) {
    RegisterFile_t regs;
    char hexBuf[256];
    
    read_registers(&regs);
    
    /* Pack registers into binary buffer */
    uint8_t regBuf[REG_COUNT * 4];
    int i;
    for (i = 0; i < 16; i++) {
        regBuf[i * 4] = regs.r[i] & 0xFF;
        regBuf[i * 4 + 1] = (regs.r[i] >> 8) & 0xFF;
        regBuf[i * 4 + 2] = (regs.r[i] >> 16) & 0xFF;
        regBuf[i * 4 + 3] = (regs.r[i] >> 24) & 0xFF;
    }
    /* xPSR */
    regBuf[64] = regs.xPSR & 0xFF;
    regBuf[65] = (regs.xPSR >> 8) & 0xFF;
    regBuf[66] = (regs.xPSR >> 16) & 0xFF;
    regBuf[67] = (regs.xPSR >> 24) & 0xFF;
    
    /* Encode as hex */
    encode_hex(regBuf, REG_COUNT * 4, hexBuf);
    strncpy(pResponse, hexBuf, maxLen - 1);
    pResponse[maxLen - 1] = '\0';
}

static void handle_read_memory(const char *pArgs, char *pResponse, uint16_t maxLen) {
    uint32_t addr, len;
    uint8_t data[64];
    int i;
    char hexBuf[256];
    
    /* Parse address,length */
    const char *comma = strchr(pArgs, ',');
    if (!comma) {
        strcpy(pResponse, "E01");
        return;
    }
    
    parse_hex_value(pArgs, &addr);
    parse_hex_value(comma + 1, &len);
    
    if (len > sizeof(data)) len = sizeof(data);
    
    /* Read memory */
    for (i = 0; i < (int)len; i++) {
        data[i] = read_memory_u8(addr + i);
    }
    
    /* Encode as hex */
    encode_hex(data, len, hexBuf);
    strncpy(pResponse, hexBuf, maxLen - 1);
    pResponse[maxLen - 1] = '\0';
}

static void handle_write_memory(const char *pArgs, char *pResponse, uint16_t maxLen) {
    uint32_t addr, len;
    const char *colon;
    uint8_t data[64];
    int dataLen;
    int i;
    
    /* Parse address,length:data */
    colon = strchr(pArgs, ':');
    if (!colon) {
        strcpy(pResponse, "E01");
        return;
    }
    
    parse_hex_value(pArgs, &addr);
    parse_hex_value(colon + 1, &len);
    
    if (len > sizeof(data)) {
        strcpy(pResponse, "E01");
        return;
    }
    
    dataLen = decode_hex(colon + 1 + 8, len * 2, data);  /* Skip "xxxxxxxx," */
    
    /* Write memory */
    for (i = 0; i < dataLen; i++) {
        write_memory_u8(addr + i, data[i]);
    }
    
    strcpy(pResponse, "OK");
}

static void handle_continue(char *pResponse, uint16_t maxLen) {
    (void)maxLen;
    
    /* Clear any pending single-step */
    s_RSPState.singleStep = false;
    s_RSPState.halted = false;
    
    debug_resume();
    
    /* Wait for halt (breakpoint, exception, etc.) - this blocks! */
    while (!s_RSPState.halted) {
        /* Could use WFI instruction here */
    }
    
    /* Return signal that caused halt */
    strcpy(pResponse, "S05");  /* SIGTRAP */
}

static void handle_step(char *pResponse, uint16_t maxLen) {
    (void)maxLen;
    
    /* Enable single-step */
    s_RSPState.singleStep = true;
    s_RSPState.halted = false;
    
    debug_step();
    
    /* Wait for step to complete */
    while (!s_RSPState.halted) {
        /* WFI */
    }
    
    strcpy(pResponse, "S05");  /* SIGTRAP */
}

static void handle_set_breakpoint(const char *pArgs, char *pResponse, uint16_t maxLen) {
    BreakpointType type;
    uint32_t addr, len;
    
    /* Parse type,addr,len */
    parse_hex_value(pArgs, &addr);
    const char *comma1 = strchr(pArgs, ',');
    if (comma1) {
        const char *comma2 = strchr(comma1 + 1, ',');
        if (comma2) {
            parse_hex_value(comma2 + 1, &len);
        }
        type = (BreakpointType)hex_to_nibble(*comma1);
    }
    
    if (set_breakpoint(type, addr) == 0) {
        strcpy(pResponse, "OK");
    } else {
        strcpy(pResponse, "E01");  /* Error */
    }
}

static void handle_remove_breakpoint(const char *pArgs, char *pResponse, uint16_t maxLen) {
    BreakpointType type;
    uint32_t addr, len;
    
    /* Parse type,addr,len */
    parse_hex_value(pArgs, &addr);
    const char *comma1 = strchr(pArgs, ',');
    if (comma1) {
        type = (BreakpointType)hex_to_nibble(*comma1);
    }
    
    if (remove_breakpoint(type, addr) == 0) {
        strcpy(pResponse, "OK");
    } else {
        strcpy(pResponse, "E01");
    }
}

static void handle_query_signal(char *pResponse, uint16_t maxLen) {
    (void)maxLen;
    strcpy(pResponse, "S05");  /* SIGTRAP - stopped by breakpoint */
}

/*---------------------------------------------------------------------------------------------------------*/
/* Public API Implementation                                                                                */
/*---------------------------------------------------------------------------------------------------------*/

void GDB_RSP_Init(void) {
    memset(&s_RSPState, 0, sizeof(s_RSPState));
    memset(s_breakpoints, 0, sizeof(s_breakpoints));
    
    /* Initial halt state for debugging */
    s_RSPState.halted = true;
    
    debug_halt();
}

bool GDB_RSP_IsHalted(void) {
    return s_RSPState.halted;
}

void GDB_RSP_Halt(void) {
    s_RSPState.halted = true;
    debug_halt();
}

void GDB_RSP_Resume(void) {
    s_RSPState.halted = false;
    debug_resume();
}

uint8_t GDB_RSP_GetSignal(void) {
    /* Determine signal that caused halt */
    return 5;  /* SIGTRAP */
}

int32_t GDB_RSP_ProcessPacket(const RSP_Packet_t *pPacket, char *pResponse, uint16_t maxLen) {
    if (!pPacket || !pResponse) return -1;
    
    pResponse[0] = '\0';
    
    switch (pPacket->type) {
        case RSP_CMD_G:
            handle_read_all_registers(pResponse, maxLen);
            break;
            
        case RSP_CMD_M:
            handle_read_memory((const char *)pPacket->data, pResponse, maxLen);
            break;
            
        case RSP_CMD_MW:
        case RSP_CMD_X:
            handle_write_memory((const char *)pPacket->data, pResponse, maxLen);
            break;
            
        case RSP_CMD_C:
            handle_continue(pResponse, maxLen);
            break;
            
        case RSP_CMD_S:
            handle_step(pResponse, maxLen);
            break;
            
        case RSP_CMD_Z_INSERT:
            handle_set_breakpoint((const char *)pPacket->data, pResponse, maxLen);
            break;
            
        case RSP_CMD_Z:
            handle_remove_breakpoint((const char *)pPacket->data, pResponse, maxLen);
            break;
            
        case RSP_CMD_Q_SUPPORTED:
            handle_query_supported(pResponse, maxLen);
            break;
            
        case RSP_CMD_Q_ATTACHED:
            handle_query_attached(pResponse, maxLen);
            break;
            
        case RSP_CMD_UNKNOWN:
        default:
            strcpy(pResponse, "");
            break;
    }
    
    return strlen(pResponse);
}

bool GDB_RSP_Parse(const uint8_t *pData, uint16_t len, RSP_Packet_t *pPacket) {
    static char packetBuf[RSP_MAX_PACKET_SIZE];
    static uint16_t packetLen = 0;
    uint16_t i;
    char csum = 0, csum1, csum2;
    
    if (!pData || !pPacket) return false;
    
    for (i = 0; i < len && packetLen < RSP_MAX_PACKET_SIZE - 1; i++) {
        char c = (char)pData[i];
        
        if (c == '$') {
            /* Start of packet */
            packetLen = 0;
            continue;
        }
        
        if (c == '#' && packetLen > 0) {
            /* End of packet, followed by checksum */
            packetBuf[packetLen] = '\0';
            
            /* Check if we have enough data for checksum */
            if (i + 2 < len) {
                /* Decode checksum */
                csum1 = hex_to_nibble(pData[i + 1]);
                csum2 = hex_to_nibble(pData[i + 2]);
                uint8_t receivedCsum = (csum1 << 4) | csum2;
                
                /* Verify checksum */
                char calcCsum = 0;
                for (int j = 0; j < packetLen; j++) {
                    calcCsum += packetBuf[j];
                }
                calcCsum = (calcCsum + (calcCsum >> 4)) & 0x0F;
                
                if (calcCsum == receivedCsum) {
                    /* Valid packet, parse command */
                    if (packetBuf[0] == 'g') {
                        pPacket->type = RSP_CMD_G;
                    } else if (packetBuf[0] == 'G') {
                        pPacket->type = RSP_CMD_G_SINGLE;
                    } else if (packetBuf[0] == 'm') {
                        pPacket->type = RSP_CMD_M;
                    } else if (packetBuf[0] == 'M') {
                        pPacket->type = RSP_CMD_MW;
                    } else if (packetBuf[0] == 'X') {
                        pPacket->type = RSP_CMD_X;
                    } else if (packetBuf[0] == 'c') {
                        pPacket->type = RSP_CMD_C;
                    } else if (packetBuf[0] == 's') {
                        pPacket->type = RSP_CMD_S;
                    } else if (packetBuf[0] == 'z') {
                        pPacket->type = RSP_CMD_Z;
                    } else if (packetBuf[0] == 'Z') {
                        pPacket->type = RSP_CMD_Z_INSERT;
                    } else if (strncmp(packetBuf, "qSupported", 10) == 0) {
                        pPacket->type = RSP_CMD_Q_SUPPORTED;
                    } else if (strncmp(packetBuf, "qAttached", 9) == 0) {
                        pPacket->type = RSP_CMD_Q_ATTACHED;
                    } else if (packetBuf[0] == '?') {
                        pPacket->type = RSP_CMD_UNKNOWN;  /* Signal query */
                    } else {
                        pPacket->type = RSP_CMD_UNKNOWN;
                    }
                    
                    /* Copy data portion */
                    if (packetLen > 1 && packetLen <= RSP_MAX_DATA_SIZE) {
                        memcpy(pPacket->data, &packetBuf[1], packetLen - 1);
                        pPacket->dataLen = packetLen - 1;
                    } else {
                        pPacket->dataLen = 0;
                    }
                    
                    packetLen = 0;
                    return true;
                }
            }
            
            packetLen = 0;
            i += 2;  /* Skip checksum digits */
            continue;
        }
        
        if (packetLen < RSP_MAX_PACKET_SIZE - 1) {
            packetBuf[packetLen++] = c;
        }
    }
    
    return false;
}
