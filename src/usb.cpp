#include "usb.h"
#include <algorithm>
#include <iostream>

// Dynamic function pointer types for WinUSB (avoid SDK header conflicts)
typedef BOOL(WINAPI* PFN_WinUsb_Initialize)(HANDLE DeviceHandle, PWINUSB_INTERFACE_HANDLE InterfaceHandle);
typedef BOOL(WINAPI* PFN_WinUsb_Free)(WINUSB_INTERFACE_HANDLE InterfaceHandle);
typedef BOOL(WINAPI* PFN_WinUsb_WritePipe)(WINUSB_INTERFACE_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, ULONG BufferLength, PULONG LengthReturned, LPOVERLAPPED Overlapped);
typedef BOOL(WINAPI* PFN_WinUsb_ReadPipe)(WINUSB_INTERFACE_HANDLE InterfaceHandle, UCHAR PipeID, PUCHAR Buffer, ULONG BufferLength, PULONG LengthReturned, LPOVERLAPPED Overlapped);
typedef BOOL(WINAPI* PFN_WinUsb_GetDescriptor)(HANDLE DeviceHandle, UCHAR DescriptorType, UCHAR Index, USHORT LanguageID, PUCHAR Buffer, ULONG BufferLength, PULONG LengthReturned);

static PFN_WinUsb_Initialize g_pfnWinUsb_Initialize = NULL;
static PFN_WinUsb_Free g_pfnWinUsb_Free = NULL;
static PFN_WinUsb_WritePipe g_pfnWinUsb_WritePipe = NULL;
static PFN_WinUsb_ReadPipe g_pfnWinUsb_ReadPipe = NULL;
static PFN_WinUsb_GetDescriptor g_pfnWinUsb_GetDescriptor = NULL;

static HMODULE g_hWinUsb = NULL;

static bool loadWinUsb()
{
    if (g_hWinUsb) return true;

    g_hWinUsb = LoadLibraryA("winusb.dll");
    if (!g_hWinUsb) {
        std::cerr << "[ERROR] Failed to load winusb.dll" << std::endl;
        return false;
    }

    g_pfnWinUsb_Initialize = (PFN_WinUsb_Initialize)GetProcAddress(g_hWinUsb, "WinUsb_Initialize");
    g_pfnWinUsb_Free = (PFN_WinUsb_Free)GetProcAddress(g_hWinUsb, "WinUsb_Free");
    g_pfnWinUsb_WritePipe = (PFN_WinUsb_WritePipe)GetProcAddress(g_hWinUsb, "WinUsb_WritePipe");
    g_pfnWinUsb_ReadPipe = (PFN_WinUsb_ReadPipe)GetProcAddress(g_hWinUsb, "WinUsb_ReadPipe");
    g_pfnWinUsb_GetDescriptor = (PFN_WinUsb_GetDescriptor)GetProcAddress(g_hWinUsb, "WinUsb_GetDescriptor");

    if (!g_pfnWinUsb_Initialize || !g_pfnWinUsb_Free || !g_pfnWinUsb_WritePipe ||
        !g_pfnWinUsb_ReadPipe || !g_pfnWinUsb_GetDescriptor) {
        std::cerr << "[ERROR] Failed to get WinUSB function addresses" << std::endl;
        FreeLibrary(g_hWinUsb);
        g_hWinUsb = NULL;
        return false;
    }

    return true;
}

// USB device descriptor (local copy based on USB spec)
#pragma pack(push, 1)
struct USB_DEVICE_DESCRIPTOR {
    UCHAR bLength;
    UCHAR bDescriptorType;
    USHORT bcdUSB;
    UCHAR bDeviceClass;
    UCHAR bDeviceSubClass;
    UCHAR bDeviceProtocol;
    UCHAR bMaxPacketSize0;
    USHORT idVendor;
    USHORT idProduct;
    USHORT bcdDevice;
    UCHAR iManufacturer;
    UCHAR iProduct;
    UCHAR iSerialNumber;
    UCHAR bNumConfigurations;
};
#pragma pack(pop)

#define USB_DEVICE_DESCRIPTOR_TYPE 0x01

// Actual GUID definition (must be defined once)
const GUID GUID_DEVINTERFACE_USB_DEVICE = { 0xA5DCBF10, 0x6530, 0x11D2, { 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED } };

UsbDevice::UsbDevice()
    : m_handle(INVALID_HANDLE_VALUE)
    , m_winusbHandle(NULL)
    , m_initialized(false)
{
}

UsbDevice::~UsbDevice()
{
    close();
}

bool UsbDevice::enumerateDevices(UINT16 vid, UINT16 pid, std::vector<UsbDeviceInfo>& devices)
{
    devices.clear();

    if (!loadWinUsb()) return false;

    // Get device info set for USB devices
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
        &GUID_DEVINTERFACE_USB_DEVICE,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERROR] SetupDiGetClassDevs failed: " << GetLastError() << std::endl;
        return false;
    }

    // Enumerate interfaces
    SP_DEVICE_INTERFACE_DATA interfaceData = {};
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, NULL, &GUID_DEVINTERFACE_USB_DEVICE, i, &interfaceData); ++i) {
        // Get interface details (need to get size first)
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, NULL, 0, &requiredSize, NULL);

        if (requiredSize == 0) continue;

        std::vector<uint8_t> buffer(requiredSize);
        SP_DEVICE_INTERFACE_DETAIL_DATA* detailData = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(buffer.data());
        detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

        if (!SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &interfaceData, detailData, requiredSize, NULL, NULL)) {
            continue;
        }

        // Open device to get VID/PID
        HANDLE hDevice = CreateFile(
            detailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (hDevice == INVALID_HANDLE_VALUE) continue;

        USB_DEVICE_DESCRIPTOR devDesc = {};
        ULONG length = 0;
        WINUSB_INTERFACE_HANDLE tempHandle = NULL;

        BOOL success = g_pfnWinUsb_Initialize(hDevice, &tempHandle);
        if (success) {
            success = g_pfnWinUsb_GetDescriptor(hDevice,
                USB_DEVICE_DESCRIPTOR_TYPE,
                0,
                0,
                (PUCHAR)&devDesc,
                sizeof(devDesc),
                &length);

            g_pfnWinUsb_Free(tempHandle);
        }

        CloseHandle(hDevice);

        if (success && devDesc.idVendor == vid && devDesc.idProduct == pid) {
            UsbDeviceInfo info = {};
            info.devicePath = detailData->DevicePath;
            info.vid = devDesc.idVendor;
            info.pid = devDesc.idProduct;
            info.description = "Nuvoton USB Mass Storage";
            devices.push_back(info);
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return !devices.empty();
}

bool UsbDevice::open(UINT16 vid, UINT16 pid)
{
    close();

    if (!loadWinUsb()) return false;

    std::vector<UsbDeviceInfo> devices;
    if (!enumerateDevices(vid, pid, devices)) {
        std::cerr << "[ERROR] No M487 USB Storage device found (VID=0x"
                  << std::hex << vid << ", PID=0x" << pid << std::dec << ")" << std::endl;
        return false;
    }

    if (devices.empty()) {
        std::cerr << "[ERROR] No devices found" << std::endl;
        return false;
    }

    // Open the first device
    m_handle = CreateFile(
        devices[0].devicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (m_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERROR] CreateFile failed: " << GetLastError() << std::endl;
        return false;
    }

    if (!g_pfnWinUsb_Initialize(m_handle, &m_winusbHandle)) {
        std::cerr << "[ERROR] WinUsb_Initialize failed: " << GetLastError() << std::endl;
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    m_initialized = true;
    std::cout << "[OK] Opened device: " << devices[0].description << std::endl;
    return true;
}

void UsbDevice::close()
{
    if (m_initialized && m_winusbHandle != NULL) {
        g_pfnWinUsb_Free(m_winusbHandle);
        m_winusbHandle = NULL;
        m_initialized = false;
    }

    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
}

bool UsbDevice::bulkWrite(const uint8_t* data, DWORD length, DWORD* bytesWritten)
{
    if (!m_initialized || m_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERROR] Device not open" << std::endl;
        return false;
    }

    ULONG written = 0;
    if (!g_pfnWinUsb_WritePipe(m_winusbHandle, BULK_OUT_EP, (PUCHAR)data, length, &written, NULL)) {
        std::cerr << "[ERROR] WinUsb_WritePipe failed: " << GetLastError() << std::endl;
        return false;
    }

    if (bytesWritten) *bytesWritten = written;
    return true;
}

bool UsbDevice::bulkRead(uint8_t* buffer, DWORD length, DWORD* bytesRead, DWORD timeoutMs)
{
    if (!m_initialized || m_handle == INVALID_HANDLE_VALUE) {
        std::cerr << "[ERROR] Device not open" << std::endl;
        return false;
    }

    ULONG read = 0;
    OVERLAPPED overlapped = {};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!overlapped.hEvent) {
        std::cerr << "[ERROR] CreateEvent failed" << std::endl;
        return false;
    }

    BOOL result = g_pfnWinUsb_ReadPipe(m_winusbHandle, BULK_IN_EP, buffer, length, &read, &overlapped);

    if (!result) {
        DWORD error = GetLastError();
        if (error == ERROR_IO_PENDING) {
            DWORD waitResult = WaitForSingleObject(overlapped.hEvent, timeoutMs);
            if (waitResult == WAIT_TIMEOUT) {
                CancelIo(m_handle);
                CloseHandle(overlapped.hEvent);
                std::cerr << "[ERROR] ReadPipe timeout" << std::endl;
                return false;
            } else if (waitResult == WAIT_OBJECT_0) {
                if (!GetOverlappedResult(m_handle, &overlapped, &read, FALSE)) {
                    CloseHandle(overlapped.hEvent);
                    std::cerr << "[ERROR] GetOverlappedResult failed" << std::endl;
                    return false;
                }
            }
        } else {
            CloseHandle(overlapped.hEvent);
            std::cerr << "[ERROR] WinUsb_ReadPipe failed: " << error << std::endl;
            return false;
        }
    }

    CloseHandle(overlapped.hEvent);
    if (bytesRead) *bytesRead = read;
    return true;
}
