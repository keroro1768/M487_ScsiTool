/**
 * @file     gdb_rsp.h
 * @brief    GDB Remote Serial Protocol (RSP) Server
 * @version  1.0.0
 * 
 * Implements GDB RSP server for self-hosted debugging via MSC Debug Channel.
 * 
 * Reference: https://sourceware.org/gdb/onlinedocs/gdb/Remote-Protocol.html
 */

#ifndef GDB_RSP_H
#define GDB_RSP_H

#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------------------------------------*/
/* RSP Constants                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define RSP_MAX_PACKET_SIZE     256
#define RSP_MAX_DATA_SIZE      (RSP_MAX_PACKET_SIZE - 6)
#define RSP_MAX_BREAKPOINTS    6

/*---------------------------------------------------------------------------------------------------------*/
/* RSP Command Types                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    RSP_CMD_NONE = 0,
    RSP_CMD_G,         /* Read all registers */
    RSP_CMD_G_SINGLE,  /* Read single register (p) */
    RSP_CMD_M,         /* Read memory */
    RSP_CMD_MW,        /* Write memory (M) */
    RSP_CMD_X,         /* Write memory (X) - binary */
    RSP_CMD_C,         /* Continue */
    RSP_CMD_S,         /* Step */
    RSP_CMD_Z,         /* Remove breakpoint/watchpoint */
    RSP_CMD_Z_INSERT,  /* Insert breakpoint/watchpoint */
    RSP_CMD_Q_SUPPORTED,   /* Query supported features */
    RSP_CMD_Q_ATTACHED,   /* Query attached state */
    RSP_CMD_Q_REVERSE_CONTINUE,  /* Query reverse continue */
    RSP_CMD_Q_REVERSE_STEP,      /* Query reverse step */
    RSP_CMD_UNKNOWN
} RSP_CMD_TYPE;

/*---------------------------------------------------------------------------------------------------------*/
/* RSP Packet Structure                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    RSP_CMD_TYPE type;
    uint8_t data[RSP_MAX_DATA_SIZE];
    uint16_t dataLen;
} RSP_Packet_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Breakpoint Types                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
typedef enum {
    BREAKPOINT_SW = 0,    /* Software breakpoint (0xBE opcode) */
    BREAKPOINT_HW = 1,    /* Hardware breakpoint (DWT) */
    WATCHPOINT_WRITE = 2, /* Write watchpoint */
    WATCHPOINT_READ = 3,  /* Read watchpoint */
    WATCHPOINT_ACCESS = 4  /* Access watchpoint */
} BreakpointType;

/*---------------------------------------------------------------------------------------------------------*/
/* RSP State                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct {
    volatile bool halted;          /* MCU is halted for debugging */
    volatile bool singleStep;      /* Single step mode */
    volatile uint32_t resumeAddr;  /* Address to resume from */
    volatile uint32_t breakpointAddr; /* Current breakpoint address */
} RSP_State_t;

/*---------------------------------------------------------------------------------------------------------*/
/* Public API                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/

/**
 * @brief   Initialize GDB RSP server
 */
void GDB_RSP_Init(void);

/**
 * @brief   Process incoming RSP packet from debug channel
 * @param   pPacket    RSP packet to process
 * @param   pResponse  Response buffer
 * @param   maxLen     Maximum response buffer length
 * @return  Response length
 */
int32_t GDB_RSP_ProcessPacket(const RSP_Packet_t *pPacket, char *pResponse, uint16_t maxLen);

/**
 * @brief   Parse RSP command from received data
 * @param   pData      Received data
 * @param   len        Data length
 * @param   pPacket    Parsed packet
 * @return  true if packet complete, false if more data needed
 */
bool GDB_RSP_Parse(const uint8_t *pData, uint16_t len, RSP_Packet_t *pPacket);

/**
 * @brief   Check if MCU should halt
 * @return  true if halted
 */
bool GDB_RSP_IsHalted(void);

/**
 * @brief   Halt the MCU (triggered by breakpoint or user)
 */
void GDB_RSP_Halt(void);

/**
 * @brief   Resume MCU execution
 */
void GDB_RSP_Resume(void);

/**
 * @brief   Get halt reason
 * @return  Signal number (e.g., 5 = SIGTRAP)
 */
uint8_t GDB_RSP_GetSignal(void);

#endif /* GDB_RSP_H */
