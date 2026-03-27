/**
 * @file     uart_debug.c
 * @brief    UART Debug Log System Implementation
 * @version  1.0.0
 * 
 * UART Debug Log provides structured logging via UART.
 * 
 * Architecture:
 *   Log macros (UART_LOG, I2C_ERR, etc.)
 *       │
 *       ▼
 *   UART_DBG_Log()  ──►  Ring Buffer  ──►  UART IRQ TX  ──►  UART0 (PB13)
 *                            (optional)
 * 
 * Without IRQ:
 *   UART_DBG_Log()  ──►  UART_DBG_PutChar() polling  ──►  UART0
 * 
 * Hardware:
 *   - UART0: PB12=RXD, PB13=TXD
 *   - Baud: 115200 (default, matches main.c)
 *   - Format: 8n1
 */

#include "NuMicro.h"
#include "uart_debug.h"

/*---------------------------------------------------------------------------------------------------------*/
/* UART Debug Configuration                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/** UART peripheral base address */
#ifndef UART_DBG_UART
#define UART_DBG_UART   UART0
#endif

/** Use UART IRQ for TX (1=IRQ, 0=polling) */
#ifndef UART_DBG_USE_IRQ
#define UART_DBG_USE_IRQ  0
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Runtime State                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
static volatile uint8_t s_u8UartDbgEnabled = 0;
static volatile UART_DBG_Level_t s_eLogLevel = UART_DBG_LEVEL_INFO;
static void *s_pUartBase = NULL;

/*---------------------------------------------------------------------------------------------------------*/
/* UART Ring Buffer (for IRQ mode)                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#if UART_DBG_USE_IRQ
static uint8_t  s_au8TxBuf[UART_DBG_BUF_SIZE];
static volatile uint16_t s_u16TxHead = 0;
static volatile uint16_t s_u16TxTail = 0;

static volatile uint8_t s_u8TxBusy = 0;
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Timestamp                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
/** Use SysTick for timestamp */
extern uint32_t SystemCoreClock;

/** Timestamp counter - milliseconds since boot */
static volatile uint32_t s_u32TickMs = 0;

/**
 * @brief  SysTick interrupt handler - used for timestamp
 * @note   Override weak definition in startup or call from SysTick_Handler
 */
void UART_DBG_SysTick_Handler(void)
{
    s_u32TickMs++;
}

/*---------------------------------------------------------------------------------------------------------*/
/* Low-Level UART Functions                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
static void _UART_PutChar(char c)
{
    UART_T *uart = (UART_T *)s_pUartBase;
    
    if (uart == NULL)
        return;
    
#if UART_DBG_USE_IRQ
    /* IRQ mode: put in ring buffer */
    uint16_t next = (s_u16TxHead + 1) % UART_DBG_BUF_SIZE;
    if (next != s_u16TxTail) {
        s_au8TxBuf[s_u16TxHead] = (uint8_t)c;
        s_u16TxHead = next;
    }
    /* Enable TX interrupt if not busy */
    if (!s_u8TxBusy) {
        s_u8TxBusy = 1;
        UART_ENABLE_INT(uart, UART_INTEN_THREIEN_Msk);
    }
#else
    /* Polling mode: wait for TX empty, then send */
    while (!(uart->FIFOSTS & UART_FIFOSTS_TXEMPTYF_Msk));
    uart->DAT = (uint8_t)c;
#endif
}

static void _UART_PutString(const char *s)
{
    if (s == NULL)
        return;
    while (*s) {
        _UART_PutChar(*s++);
    }
}

/*---------------------------------------------------------------------------------------------------------*/
/* UART IRQ Handler (for buffered TX mode)                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
#if UART_DBG_USE_IRQ
void UART_DBG_IRQHandler(void)
{
    UART_T *uart = (UART_T *)s_pUartBase;
    uint32_t u32IntFlag = UART_GetIntFlag(uart, UART_INTSTS_THREINT_Msk);
    
    if (u32IntFlag) {
        /* TX holding register empty */
        if (s_u16TxHead != s_u16TxTail) {
            /* More data to send */
            UART_WRITE(uart, s_au8TxBuf[s_u16TxTail]);
            s_u16TxTail = (s_u16TxTail + 1) % UART_DBG_BUF_SIZE;
        } else {
            /* Buffer empty, disable TX interrupt */
            s_u8TxBusy = 0;
            UART_DISABLE_INT(uart, UART_INTEN_THREIEN_Msk);
        }
    }
}
#endif

/*---------------------------------------------------------------------------------------------------------*/
/* Timestamp                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void UART_DBG_Timestamp(char *buf, int size)
{
    uint8_t hour, min, sec, ms;
    uint32_t t;
    
    if (buf == NULL || size < 12)
        return;
    
    t = s_u32TickMs;
    hour = (t / 3600000) % 24;
    min  = (t / 60000) % 60;
    sec  = (t / 1000) % 60;
    ms   = t % 1000;
    
    snprintf(buf, size, "%02d:%02d:%02d.%03d", hour, min, sec, ms);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Core Log Functions                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
void UART_DBG_Init(void *uartBase, uint32_t sysclk)
{
    UART_T *uart = (UART_T *)uartBase;
    
    s_pUartBase = uartBase;
    s_u32TickMs = 0;
    
    if (uart == NULL)
        return;
    
#if UART_DBG_USE_IRQ
    /* Configure UART interrupt */
    UART_EnableInt(uart, UART_INTEN_THREIEN_Msk);
    NVIC_EnableIRQ(UART0_IRQn);
#endif
    
    s_u8UartDbgEnabled = 1;
    
    /* Welcome message */
    _UART_PutString("\r\n");
    _UART_PutString("========================================\r\n");
    _UART_PutString("  UART Debug Log Initialized\r\n");
    _UART_PutString("  Baud: 115200, 8n1\r\n");
    _UART_PutString("  Level: INFO (");
#if UART_LOG_LEVEL == 0
    _UART_PutString("ERR only)\r\n");
#elif UART_LOG_LEVEL == 1
    _UART_PutString("WARN+)\r\n");
#elif UART_LOG_LEVEL == 2
    _UART_PutString("INFO+)\r\n");
#elif UART_LOG_LEVEL == 3
    _UART_PutString("DBG+)\r\n");
#else
    _UART_PutString("TRACE)\r\n");
#endif
    _UART_PutString("========================================\r\n");
}

void UART_DBG_Enable(uint8_t enable)
{
    s_u8UartDbgEnabled = enable ? 1 : 0;
}

void UART_DBG_SetLevel(UART_DBG_Level_t level)
{
    s_eLogLevel = level;
}

UART_DBG_Level_t UART_DBG_GetLevel(void)
{
    return s_eLogLevel;
}

void UART_DBG_Flush(void)
{
#if UART_DBG_USE_IRQ
    /* Wait for TX buffer to empty */
    while (s_u16TxHead != s_u16TxTail);
#endif
}

/*---------------------------------------------------------------------------------------------------------*/
/* Minimal vsnprintf (for embedded)                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
static int _UART_Vsnprintf(char *buf, int size, const char *fmt, va_list ap)
{
    char c, *s;
    int n = 0;
    int field_width;
    int precision;
    int len;
    char tmp[16];
    
    if (buf == NULL || size <= 0)
        return 0;
    
    while (*fmt && n < (size - 1)) {
        if (*fmt != '%') {
            buf[n++] = *fmt++;
            continue;
        }
        
        fmt++;  /* skip % */
        
        /* Reset flags */
        field_width = 0;
        precision = -1;
        
        /* Parse field width */
        if (*fmt >= '0' && *fmt <= '9') {
            field_width = 0;
            while (*fmt >= '0' && *fmt <= '9')
                field_width = field_width * 10 + (*fmt++ - '0');
        } else if (*fmt == '*') {
            field_width = va_arg(ap, int);
            fmt++;
        }
        
        /* Parse precision */
        if (*fmt == '.') {
            fmt++;
            if (*fmt >= '0' && *fmt <= '9') {
                precision = 0;
                while (*fmt >= '0' && *fmt <= '9')
                    precision = precision * 10 + (*fmt++ - '0');
            } else if (*fmt == '*') {
                precision = va_arg(ap, int);
                fmt++;
            }
        }
        
        c = *fmt++;
        
        switch (c) {
        case 'c':
            tmp[0] = (char)va_arg(ap, int);
            tmp[1] = '\0';
            len = 1;
            s = tmp;
            break;
            
        case 's':
            s = va_arg(ap, char *);
            if (s == NULL)
                s = "(null)";
            len = strlen(s);
            if (precision >= 0 && len > precision)
                len = precision;
            break;
            
        case 'd':
        case 'i':
            len = snprintf(tmp, sizeof(tmp), "%d", va_arg(ap, int));
            s = tmp;
            break;
            
        case 'u':
            len = snprintf(tmp, sizeof(tmp), "%u", va_arg(ap, unsigned int));
            s = tmp;
            break;
            
        case 'l':
        case 'z':
            if (fmt[0] == 'd') {
                fmt++;
                len = snprintf(tmp, sizeof(tmp), "%ld", va_arg(ap, long));
            } else if (fmt[0] == 'u') {
                fmt++;
                len = snprintf(tmp, sizeof(tmp), "%lu", va_arg(ap, unsigned long));
            } else {
                len = 0;
            }
            s = tmp;
            break;
            
        case 'x':
        case 'X':
            if (field_width == 0) {
                len = snprintf(tmp, sizeof(tmp), "%x", va_arg(ap, unsigned int));
            } else {
                char fmt2[8];
                snprintf(fmt2, sizeof(fmt2), "0%dx", field_width);
                len = snprintf(tmp, sizeof(tmp), fmt2, va_arg(ap, unsigned int));
            }
            s = tmp;
            break;
            
        case 'p':
            len = snprintf(tmp, sizeof(tmp), "0x%08X", va_arg(ap, unsigned long));
            s = tmp;
            break;
            
        case '%':
            buf[n++] = '%';
            continue;
            
        default:
            buf[n++] = '%';
            buf[n++] = c;
            continue;
        }
        
        /* Output the string */
        for (int i = 0; i < len && n < (size - 1); i++)
            buf[n++] = s[i];
    }
    
    buf[n] = '\0';
    return n;
}

void UART_DBG_Log(const char *fmt, ...)
{
    char buffer[128];
    va_list ap;
    int len;
    char ts[16];
    
    if (!s_u8UartDbgEnabled)
        return;
    
    /* Add timestamp */
    UART_DBG_Timestamp(ts, sizeof(ts));
    _UART_PutString("[");
    _UART_PutString(ts);
    _UART_PutString("] ");
    
    /* Format the message */
    va_start(ap, fmt);
    len = _UART_Vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    
    /* Output via UART */
    for (int i = 0; i < len; i++)
        _UART_PutChar(buffer[i]);
}

/*---------------------------------------------------------------------------------------------------------*/
/* Hex Dump                                                                                                */
/*---------------------------------------------------------------------------------------------------------*/
void UART_DBG_HexDump(const char *label, const uint8_t *data, int len)
{
    char line[80];
    char ts[16];
    int i;
    int col;
    
    if (!s_u8UartDbgEnabled || data == NULL || len <= 0)
        return;
    
    /* Timestamp */
    UART_DBG_Timestamp(ts, sizeof(ts));
    _UART_PutString("[");
    _UART_PutString(ts);
    _UART_PutString("] ");
    
    /* Label */
    if (label) {
        _UART_PutString("[");
        _UART_PutString(label);
        _UART_PutString("] ");
    }
    
    _UART_PutString("Hex Dump: ");
    
    /* Format: XX XX XX ... (16 bytes per line) */
    for (i = 0; i < len; i++) {
        static const char hex_chars[] = "0123456789ABCDEF";
        _UART_PutChar(hex_chars[(data[i] >> 4) & 0x0F]);
        _UART_PutChar(hex_chars[data[i] & 0x0F]);
        _UART_PutChar(' ');
        
        if (((i + 1) % 16) == 0 && i < len - 1) {
            _UART_PutString("\r\n[");
            _UART_PutString(ts);
            _UART_PutString("]       ");
        }
    }
    
    _UART_PutString("\r\n");
}

/*---------------------------------------------------------------------------------------------------------*/
/* Direct Low-Level Output                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
void UART_DBG_PutChar(char c)
{
    if (!s_u8UartDbgEnabled)
        return;
    _UART_PutChar(c);
}

void UART_DBG_PutString(const char *s)
{
    if (!s_u8UartDbgEnabled || s == NULL)
        return;
    _UART_PutString(s);
}
