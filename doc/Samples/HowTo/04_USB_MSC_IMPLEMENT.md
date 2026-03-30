# USB Mass Storage 實作

## 📋 概述

M487 的 HSUSBD（USB High Speed Device）支援 USB MSC（Mass Storage Class），可以將裝置模擬為 USB 磁碟機。

---

## 🔧 核心變數與陣列

位置：`M480BSP\SampleCode\StdDriver\HSUSBD_Mass_Storage_ShortPacket\MassStorage.c`

```c
/* RAM 磁碟大小：2048 * 512 = 1MB */
#define STORAGE_DISK_SIZE (2048 * 512)

/* RAM 磁碟區（實際存放檔案資料的地方）*/
static uint8_t g_au8StorageDisk[STORAGE_DISK_SIZE];
```

---

## 📖 主要函式

### MSD_Init() — 初始化

```c
int MSD_Init(void)
{
    /* 設定 SCSI Inquiry Data（回傳給 PC 的裝置資訊）*/
    /* 廠商：Nuvoton, 產品：UMass Device, 版本：1.0 */
    ...
}
```

### MSD_Read() — 讀取磁碟資料（PC → M487）

```c
int MSD_Read(uint32_t addr, uint32_t size, uint8_t *pData)
{
    uint32_t i;

    if((addr + size) > STORAGE_DISK_SIZE)
        return FALSE;

    /* 從 RAM 磁碟陣列讀取 */
    for(i = 0; i < size; i++)
        pData[i] = g_au8StorageDisk[addr + i];

    return TRUE;
}
```

### MSD_Write() — 寫入磁碟資料（M487 → PC）

```c
int MSD_Write(uint32_t addr, uint32_t size, uint8_t *pData)
{
    uint32_t i;

    if((addr + size) > STORAGE_DISK_SIZE)
        return FALSE;

    /* 寫入 RAM 磁碟陣列 */
    for(i = 0; i < size; i++)
        g_au8StorageDisk[addr + i] = pData[i];

    return TRUE;
}
```

### MSD_Capacity() — 回傳磁碟容量

```c
int MSD_Capacity(uint32_t *pu32SectorCount, uint32_t *pu32SectorSize)
{
    *pu32SectorCount = STORAGE_DISK_SIZE / 512;  // 2048 sectors
    *pu32SectorSize  = 512;                       // 每 sector 512 bytes
    return TRUE;
}
```

---

## 🔒 Write Protect（寫入保護）

### 啟用嚴格保護

將 `gKeepMediaWriteProtected` 設為 `1`：

```c
static volatile int gKeepMediaWriteProtected = 1;  // 1 = 保護開啟
```

此時 `MSD_Write()` 會直接返回 `FALSE`，PC 會收到「媒體寫入保護」錯誤。

### 啟用假寫入（使用者體驗較好）

PC 認為寫入成功，但實際上沒寫入：

```c
int MSD_Write(uint32_t addr, uint32_t size, uint8_t *pData)
{
    uint32_t i;

    if((addr + size) > STORAGE_DISK_SIZE)
        return FALSE;

    /* 假寫入：返回成功但不做任何事 */
    /* PC 顯示複製成功，但檔案不會真的寫入 */
    return TRUE;  // 或 return -1，取決於錯誤處理
}
```

### 組合方案（最佳）

```c
int MSD_Write(uint32_t addr, uint32_t size, uint8_t *pData)
{
    if(gKeepMediaWriteProtected)
    {
        /* 寫入保護模式：真的寫入，但唯讀 */
        if(addr >= READ_ONLY_boundary)
            return FALSE;
    }

    /* 正常寫入 */
    for(i = 0; i < size; i++)
        g_au8StorageDisk[addr + i] = pData[i];

    return TRUE;
}
```

---

## 🔌 USB Descriptor（VID/PID）

位置：`descriptors.c`

```c
#define USBD_VID            0x0416
#define USBD_PID            0xFF20
#define USBD_VCP_EN         0

const S_USBD_DESC_T code LSUSBD_Descriptor =
{
    USBD_VID,           // Vendor ID
    USBD_PID,           // Product ID
    0x0100,             // bcdUSB (1.10)
    0x00,               // bDeviceClass
    ...
};
```

燒錄 VID=0x0416, PID=0xFF20 的 MSC 程式後，PC 會識別為 `Nuvoton UMass Device`。

---

## 📊 記憶體使用

| 項目 | 大小 | 位址 |
|------|------|------|
| g_au8StorageDisk[] | 1MB | 0x20000000+ (SRAM) |
| USB Descriptor | ~128B | Flash |
| MSC SCSI Data | ~36B | Flash |

> ⚠️ M487 SRAM 為 160KB，1MB RAM 磁碟會超出。需要根據實際 SRAM 大小調整 STORAGE_DISK_SIZE。

---

## 🔧 修改並重新編譯燒錄流程

### 步驟 1：修改程式碼

編輯 `MassStorage.c` 中的 `MSD_Read()` 和 `MSD_Write()`

### 步驟 2：編譯

```
F7（Keil GUI）或命令列
```

### 步驟 3：燒錄

```powershell
& "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\bin\openocd.exe" `
  -s "C:\Users\rinry\Tool\OpenOCD-Nuvoton\OpenOCD\scripts" `
  -f interface/nulink.cfg -f target/numicroM4.cfg `
  -c "init" -c "reset halt" `
  -c "flash write_image erase C:/AiWorkSpace/KM/M480BSP/SampleCode/StdDriver/HSUSBD_Mass_Storage_ShortPacket/KEIL/obj/HSUSBD_Mass_Storage_ShortPacket.bin 0" `
  -c "shutdown"
```

### 步驟 4：測試

重新插拔 M487 USB，確認 PC 辨識為 USB 磁碟，嘗試複製檔案進去。
