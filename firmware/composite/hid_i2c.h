/**
 * @file     hid_i2c.h
 * @brief    M487 USB Composite Device - HID I2C Bridge + MSC Header
 * @version  2.0.0
 * 
 * NOTE: Most USB descriptor macros (LEN_*, DESC_*, EP_*, etc.) are defined
 *       in the BSP headers (usbd.h, massstorage.h) which are included via NuMicro.h.
 */

#ifndef __HID_I2C_H__
#define __HID_I2C_H__

#include <stdint.h>

/*---------------------------------------------------------------------------------------------------------*/
/* USB VID/PID (project-specific)                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define USBD_VID                0x0416
#define USBD_PID                0x5020

/*---------------------------------------------------------------------------------------------------------*/
/* USB EP Max Packet Sizes & Buffer Addresses                                                              */
/*  (These may be duplicated from usbd.h; use BSP values if defined)                                     */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef CEP_MAX_PKT_SIZE
#define CEP_MAX_PKT_SIZE        64
#endif
#ifndef EPA_MAX_PKT_SIZE
#define EPA_MAX_PKT_SIZE        512
#endif
#ifndef EPB_MAX_PKT_SIZE
#define EPB_MAX_PKT_SIZE        512
#endif
#ifndef EPC_MAX_PKT_SIZE
#define EPC_MAX_PKT_SIZE        512
#endif
#ifndef EPD_MAX_PKT_SIZE
#define EPD_MAX_PKT_SIZE        512
#endif

#ifndef EPA_OTHER_MAX_PKT_SIZE
#define EPA_OTHER_MAX_PKT_SIZE  64
#endif
#ifndef EPB_OTHER_MAX_PKT_SIZE
#define EPB_OTHER_MAX_PKT_SIZE  64
#endif
#ifndef EPC_OTHER_MAX_PKT_SIZE
#define EPC_OTHER_MAX_PKT_SIZE  64
#endif
#ifndef EPD_OTHER_MAX_PKT_SIZE
#define EPD_OTHER_MAX_PKT_SIZE  64
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* EP Buffer Addresses                                                                                      */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef CEP_BUF_BASE
#define CEP_BUF_BASE    0
#define CEP_BUF_LEN    CEP_MAX_PKT_SIZE
#endif
#ifndef EPA_BUF_BASE
#define EPA_BUF_BASE    0x200
#define EPA_BUF_LEN    EPA_MAX_PKT_SIZE
#endif
#ifndef EPB_BUF_BASE
#define EPB_BUF_BASE    0x400
#define EPB_BUF_LEN    EPB_MAX_PKT_SIZE
#endif
#ifndef EPC_BUF_BASE
#define EPC_BUF_BASE    0x600
#define EPC_BUF_LEN    EPC_MAX_PKT_SIZE
#endif
#ifndef EPD_BUF_BASE
#define EPD_BUF_BASE    0x800
#define EPD_BUF_LEN    EPD_MAX_PKT_SIZE
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* EP Numbers                                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef INT_IN_EP_NUM
#define INT_IN_EP_NUM       0x01
#endif
#ifndef INT_OUT_EP_NUM
#define INT_OUT_EP_NUM     0x02
#endif
#ifndef BULK_IN_EP_NUM
#define BULK_IN_EP_NUM     0x03
#endif
#ifndef BULK_OUT_EP_NUM
#define BULK_OUT_EP_NUM    0x04
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* USB Configuration                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef HID_DEFAULT_INT_IN_INTERVAL
#define HID_DEFAULT_INT_IN_INTERVAL  4
#endif
#ifndef USBD_SELF_POWERED
#define USBD_SELF_POWERED           0
#endif
#ifndef USBD_REMOTE_WAKEUP
#define USBD_REMOTE_WAKEUP          0
#endif
#ifndef USBD_MAX_POWER
#define USBD_MAX_POWER              50
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* MSC BOT States                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef BULK_NORMAL
#define BULK_NORMAL     0xFF
#endif
#ifndef BULK_CBW
#define BULK_CBW        0x00
#endif
#ifndef BULK_OUT
#define BULK_OUT        0x02
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* MSC Data Transfer                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef USBD_MAX_DMA_LEN
#define USBD_MAX_DMA_LEN     0x200
#endif
#ifndef USBD_SECTOR_SIZE
#define USBD_SECTOR_SIZE     512
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* HID I2C Commands (project-specific)                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
#define HID_CMD_SIGNATURE       0x43444948  /* "HDIB" little-endian */
#define HID_CMD_I2C_WRITE       0x01
#define HID_CMD_I2C_READ        0x02
#define HID_CMD_I2C_WRITEREAD   0x03
#define HID_CMD_I2C_SCAN        0x04
#define HID_CMD_NONE            0x00

#define I2C_MAX_WRITE_LEN       60
#define I2C_MAX_READ_LEN        62
#define PAGE_SIZE               512

/*---------------------------------------------------------------------------------------------------------*/
/* HID Command Structure (project-specific)                                                                */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)
typedef struct {
    uint8_t  u8Cmd;
    uint8_t  u8Size;
    uint8_t  u8Data[252];
    uint32_t u32Signature;
    uint32_t u32Checksum;
} CMD_T;
#pragma pack(pop)

/*---------------------------------------------------------------------------------------------------------*/
/* MSC Structures                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#pragma pack(push, 1)
typedef struct {
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t  bmCBWFlags;
    uint8_t  bCBWLUN;
    uint8_t  bCBWCBLength;
    uint8_t  u8OPCode;
    uint8_t  u8LUN;
    uint8_t  au8Data[15];
} CBW_t;

typedef struct {
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t  bCSWStatus;
} CSW_t;
#pragma pack(pop)

#define CBW_SIGNATURE   0x43425355
#define CSW_SIGNATURE   0x53425355

/*---------------------------------------------------------------------------------------------------------*/
/* MSC Class Requests                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef BULK_ONLY_MASS_STORAGE_RESET
#define BULK_ONLY_MASS_STORAGE_RESET   0xFF
#endif
#ifndef GET_MAX_LUN
#define GET_MAX_LUN                     0xFE
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* HID Class Requests                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef GET_REPORT
#define GET_REPORT              0x01
#endif
#ifndef GET_IDLE
#define GET_IDLE                0x02
#endif
#ifndef GET_PROTOCOL
#define GET_PROTOCOL            0x03
#endif
#ifndef SET_REPORT
#define SET_REPORT              0x09
#endif
#ifndef SET_IDLE
#define SET_IDLE                0x0A
#endif
#ifndef SET_PROTOCOL
#define SET_PROTOCOL            0x0B
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* MSC SCSI Opcodes                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef UFI_TEST_UNIT_READY
#define UFI_TEST_UNIT_READY         0x00
#endif
#ifndef UFI_REQUEST_SENSE
#define UFI_REQUEST_SENSE           0x03
#endif
#ifndef UFI_INQUIRY
#define UFI_INQUIRY                0x12
#endif
#ifndef UFI_MODE_SENSE_6
#define UFI_MODE_SENSE_6           0x1A
#endif
#ifndef UFI_START_STOP
#define UFI_START_STOP             0x1B
#endif
#ifndef UFI_MEDIA_REMOVAL
#define UFI_MEDIA_REMOVAL          0x1E
#endif
#ifndef UFI_READ_FORMAT_CAPACITY
#define UFI_READ_FORMAT_CAPACITY    0x23
#endif
#ifndef UFI_READ_CAPACITY
#define UFI_READ_CAPACITY          0x25
#endif
#ifndef UFI_READ_10
#define UFI_READ_10                0x28
#endif
#ifndef UFI_WRITE_10
#define UFI_WRITE_10               0x2A
#endif
#ifndef UFI_VERIFY_10
#define UFI_VERIFY_10              0x2F
#endif
#ifndef UFI_MODE_SELECT_10
#define UFI_MODE_SELECT_10          0x55
#endif
#ifndef UFI_MODE_SENSE_10
#define UFI_MODE_SENSE_10          0x5A
#endif
#ifndef UFI_PREVENT_ALLOW_MEDIUM_REMOVAL
#define UFI_PREVENT_ALLOW_MEDIUM_REMOVAL  0x1E
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables (extern)                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
extern uint8_t volatile g_u8MscStart;
extern uint8_t g_u8OutBuff[512];  /* EPB_MAX_PKT_SIZE */
extern CBW_t g_sCBW;
extern CSW_t g_sCSW;

/*---------------------------------------------------------------------------------------------------------*/
/* USB HID functions                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void HID_Init(void);
void HID_InitForHighSpeed(void);
void HID_InitForFullSpeed(void);
void HID_ClassRequest(void);
void HID_VendorRequest(void);
void HID_GetOutReport(uint8_t *pu8EpBuf, uint32_t u32Size);
void HID_SetInReport(void);
void EPA_Handler(void);
void EPB_Handler(void);

/*---------------------------------------------------------------------------------------------------------*/
/* MSC functions                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void MSC_ProcessCmd(void);
void MSC_ReceiveCBW(uint32_t u32Buf);
void MSC_BulkOut(uint32_t u32Addr, uint32_t u32Len);
void MSC_BulkIn(uint32_t u32Addr, uint32_t u32Len);
void MSC_ActiveDMA(uint32_t u32Addr, uint32_t u32Len);
void MSC_AckCmd(uint32_t u32Residue);

/*---------------------------------------------------------------------------------------------------------*/
/* I2C functions (project-specific)                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
void I2C0_Init(void);
int32_t I2C_Write(uint8_t slaveAddr, const uint8_t *data, uint16_t len);
int32_t I2C_Read(uint8_t slaveAddr, uint8_t *data, uint16_t len);
int32_t I2C_WriteRead(uint8_t slaveAddr, const uint8_t *wdata, uint8_t wlen, uint8_t *rdata, uint8_t rlen);
int32_t I2C_Scan(uint8_t *foundAddrs, uint8_t maxCount);

/*---------------------------------------------------------------------------------------------------------*/
/* Utility functions                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
uint32_t CalCheckSum(uint8_t *buf, uint32_t size);

#endif /* __HID_I2C_H__ */
