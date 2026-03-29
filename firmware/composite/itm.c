/**
 * @file     itm.c
 * @brief    ITM (Instrumentation Trace Macrocell) SWO Trace Implementation
 * @version  1.0.0
 * 
 * ITM Trace System for M487 Cortex-M4
 * SWO pin: PB8 (Single Wire Output)
 * 
 * Hardware:
 *   - ARM Cortex-M4 CoreSight ITM (Instrumentation Trace Macrocell)
 *   - TPI (Trace Port Interface) for SWO output
 *   - PB8 = SWO (verify M487 datasheet for exact pin)
 * 
 * Reference:
 *   - ARMv7-M Architecture Reference Manual (DDI0403E)
 *   - ARM Cortex-M4 Technical Reference Manual
 *   - M487 User Manual (Nuvoton)
 */

#include "NuMicro.h"
#include "itm.h"

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Runtime State                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
static volatile uint8_t s_u8ItmEnabled = 0;
static volatile ITM_LogLevel_t s_eLogLevel = ITM_LOG_LEVEL;

/*---------------------------------------------------------------------------------------------------------*/
/* TPI SWO Baud Rate Configuration                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * TPI ACPR Prescaler calculation:
 *   SWO freq = core_clk / (SWOPrescaler * 2)
 *   
 *   M487 @ 192 MHz PLL clock:
 *     core_clk = 192 MHz
 *     SWOPrescaler = 7  →  SWO freq = 13.7 MHz
 *     SWOPrescaler = 15 →  SWO freq = 6.4 MHz
 *     SWOPrescaler = 31 →  SWO freq = 3.1 MHz
 *     SWOPrescaler = 95 →  SWO freq = 1.0 MHz
 *     SWOPrescaler = 191 → SWO freq = 500 KHz
 * 
 * Common debuggers (J-Link, CMSIS-DAP) support up to 10 MHz SWO.
 */

#ifndef ITM_SWO_CORE_CLOCK
#define ITM_SWO_CORE_CLOCK   192000000UL   /* M487 PLL clock = 192 MHz */
#endif

#ifndef ITM_SWO_BAUD
#define ITM_SWO_BAUD         2000000UL     /* 2 MHz SWO baud rate */
#endif

/**
 * Calculate SWOPrescaler from desired baud rate:
 *   SWOPrescaler = (core_clk / baud) / 2 - 1
 */
#define ITM_calc_prescaler(baud)  (((ITM_SWO_CORE_CLOCK) / (baud)) / 2 - 1)

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Init                                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
void ITM_Init(void)
{
    ITM_InitWithBaud(ITM_SWO_BAUD);
}

void ITM_InitWithBaud(uint32_t swo_freq_hz)
{
    uint32_t prescaler;
    
    /* Calculate prescaler */
    prescaler = ITM_calc_prescaler(swo_freq_hz);
    if (prescaler > 0xFFFF)
        prescaler = 0xFFFF;
    if (prescaler < 1)
        prescaler = 1;
    
    /* Unlock ITM registers */
    ITM->LAR = 0xC5ACCE55;
    
    /* Disable ITM and all stimulus ports during configuration */
    ITM->TCR = 0;
    ITM->TER = 0;
    
    /* Configure TPI (Trace Port Interface) for SWO */
    /* TPI->ACPR = prescaler divider for SWO baud rate */
    TPI->ACPR = (uint32_t)prescaler;
    
    /* TPI->SPPR = 2 (SWO Manchester protocol, 1.0 Mbps max recommended) */
    /* Default is Sync mode (0x02), which uses 4B/5B encoding - use default */
    TPI->SPPR = 0x02;  /* SWO NRZ / Async mode (set to 0x01 if needed for specific debuggers) */
    
    /* TPI->FFCR - formatting control */
    TPI->FFCR = 0x00;   /* No formatting, raw data */
    
    /* Flush trace data */
    volatile uint32_t tmp;
    while (TPI->FFSR & TPI_FFSR_FlInProg_Msk) {
        tmp = TPI->FIFO1; (void)tmp;
    }
    (void)tmp;
    
    /* Enable ITM stimulus port 0 */
    ITM->TER = (1 << ITM_STIM_PORT);
    
    /* Configure ITM Trace Control Register */
    /* ITM_TCR:
         Bit 0:  ITMENA    - Enable ITM
         Bit 1:  TSENA     - Enable timestamp generation
         Bit 2:  SYNCENA    - Enable sync packet generation
         Bit 3:  DWTENA    - Enable DWT trigger
         Bit 4:  SWOENA    - Enable SWO output (through TPI)
         Bit 7:  BusErrorEn - Enable bus error detection
    */
    ITM->TCR = (0 << 0)   /* ITMENA - will enable after PB8 setup */
               | (1 << 1)   /* TSENA - timestamps */
               | (0 << 2)   /* SYNCENA - disable sync (overhead) */
               | (0 << 3)   /* DWTENA - disable DWT trigger */
               | (1 << 4)   /* SWOENA - enable SWO output */
               | (0 << 7);  /* BusErrorEn */
    
    /* Configure ITM Trace Privilege Register */
    /* ITM_TPR: Set unprivileged (user) access to all stimulus ports */
    ITM->TPR = 0;
    
    /* Enable ITM finally */
    ITM->TCR |= (1 << 0);  /* Set ITMENA */
    
    /* Enable DWT cycle counter for timestamps */
    /* DWT->CTRL: Set CYCCNTENA bit (bit 0) to enable cycle counter */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* Enable trace access */
    DWT->CTRL |= (1 << 0);  /* CYCCNTENA */
    
    s_u8ItmEnabled = 1;
    
    /* Send boot marker via ITM */
    ITM_LOG("\n");
    ITM_LOG("========================================\n");
    ITM_LOG("  ITM Trace Initialized\n");
    ITM_LOG("  SWO freq: %lu Hz\n", (unsigned long)(ITM_SWO_CORE_CLOCK / ((prescaler + 1) * 2)));
    ITM_LOG("  Core clock: %lu Hz\n", (unsigned long)ITM_SWO_CORE_CLOCK);
    ITM_LOG("========================================\n");
    ITM_LOG("\n");
}

void ITM_Enable(uint8_t enable)
{
    if (enable) {
        ITM->LAR = 0xC5ACCE55;  /* Unlock ITM */
        ITM->TCR |= (1 << 0);   /* Enable ITM */
        s_u8ItmEnabled = 1;
    } else {
        ITM->TCR &= ~(1 << 0); /* Disable ITM */
        s_u8ItmEnabled = 0;
    }
}

void ITM_SetLevel(ITM_LogLevel_t level)
{
    s_eLogLevel = level;
}

ITM_LogLevel_t ITM_GetLevel(void)
{
    return s_eLogLevel;
}

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Timestamp                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void ITM_Timestamp(char *buf, int size)
{
    uint32_t u32Tick;
    uint8_t hour, min, sec, ms;
    
    if (buf == NULL || size < 12)
        return;
    
    /* Use DWT cycle counter for timestamp */
    u32Tick = DWT->CYCCNT;
    
    /* Convert cycle count to time assuming 192 MHz */
    /* NOTE: This is a simplified timestamp - actual time tracking should use RTC or SysTick */
    uint32_t total_ms = u32Tick / 192000UL;
    uint32_t us = (u32Tick % 192000UL) / 192UL;  /* microseconds part */
    
    hour = (total_ms / 3600000) % 24;
    min  = (total_ms / 60000) % 60;
    sec  = (total_ms / 1000) % 60;
    ms   = total_ms % 1000;
    
    snprintf(buf, size, "%02d:%02d:%02d.%03d",
             hour, min, sec, ms);
}

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Formatted Log                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
/**
 * @brief  Internal function to output a character via ITM
 */
static void _ITM_PutChar(char c)
{
    if (!s_u8ItmEnabled)
        return;
    
    /* Wait until ITM stimulus port is ready */
    while (ITM->PORT[ITM_STIM_PORT].u32 == 0);
    ITM->PORT[ITM_STIM_PORT].u8 = (uint8_t)c;
}

/**
 * @brief  Internal function to output a string via ITM
 */
static void _ITM_PutString(const char *str)
{
    if (!s_u8ItmEnabled || str == NULL)
        return;
    
    while (*str) {
        /* Wait until ready */
        while (ITM->PORT[ITM_STIM_PORT].u32 == 0);
        ITM->PORT[ITM_STIM_PORT].u8 = (uint8_t)(*str++);
    }
}

/**
 * @brief  Minimal vsnprintf implementation for ITM
 */
static int _ITM_Vsnprintf(char *buf, int size, const char *fmt, va_list ap)
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
                /* Handle %02x, %04x style */
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

void ITM_Log(const char *fmt, ...)
{
    char buffer[128];
    va_list ap;
    int len;
    char ts_buf[16];
    
    if (!s_u8ItmEnabled)
        return;
    
    /* Add timestamp */
    ITM_Timestamp(ts_buf, sizeof(ts_buf));
    _ITM_PutString(ts_buf);
    _ITM_PutString(": ");
    
    /* Format the message */
    va_start(ap, fmt);
    len = _ITM_Vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    
    /* Output via ITM */
    for (int i = 0; i < len; i++)
        _ITM_PutChar(buffer[i]);
}

/*---------------------------------------------------------------------------------------------------------*/
/* ITM Hex Dump                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
void ITM_HexDump(const char *label, const uint8_t *data, int len)
{
    char line[80];
    char ts_buf[16];
    int i;
    
    if (!s_u8ItmEnabled || data == NULL || len <= 0)
        return;
    
    /* Timestamp */
    ITM_Timestamp(ts_buf, sizeof(ts_buf));
    _ITM_PutString(ts_buf);
    _ITM_PutString(": ");
    
    /* Label */
    if (label) {
        _ITM_PutString("[");
        _ITM_PutString(label);
        _ITM_PutString("] ");
    }
    
    /* Hex dump */
    for (i = 0; i < len; i++) {
        static const char hex_chars[] = "0123456789ABCDEF";
        
        if ((i & 0x0F) == 0) {
            /* New line every 16 bytes */
            if (i > 0) {
                /* ASCII representation */
                _ITM_PutString("  ");
                for (int j = i - 16; j < i; j++) {
                    char c = (data[j] >= 32 && data[j] < 127) ? data[j] : '.';
                    _ITM_PutChar(c);
                }
            }
            _ITM_PutString("\n");
            _ITM_PutString(ts_buf);
            _ITM_PutString(": ");
            _ITM_PutString("0000");  /* Would normally show offset */
            _ITM_PutString("  ");
        } else if ((i & 0x07) == 0) {
            _ITM_PutChar(' ');
        }
        
        _ITM_PutChar(hex_chars[(data[i] >> 4) & 0x0F]);
        _ITM_PutChar(hex_chars[data[i] & 0x0F]);
        _ITM_PutChar(' ');
    }
    
    /* Pad last line */
    int remaining = 16 - (len & 0x0F);
    if (remaining < 16) {
        for (int j = 0; j < remaining; j++) {
            _ITM_PutString("   ");
        }
        if (remaining > 8)
            _ITM_PutChar(' ');
        /* ASCII */
        _ITM_PutString("  ");
        for (int j = (len & ~0x0F); j < len; j++) {
            char c = (data[j] >= 32 && data[j] < 127) ? data[j] : '.';
            _ITM_PutChar(c);
        }
    }
    
    _ITM_PutString("\n");
}
