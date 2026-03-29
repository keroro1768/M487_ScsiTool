/*
 * M487FilterWmiApp.c
 *
 * User-mode application for querying M487 HID Filter Driver via WMI.
 * Demonstrates WMI Query/SET Instance operations for ring buffer access.
 *
 * Build:
 *   cl /EHsc /W4 M487FilterWmiApp.c uuid.lib ole32.lib oleaut32.lib
 *
 * Usage:
 *   M487FilterWmiApp.exe [--query | --set | --monitor] [options]
 *
 * Examples:
 *   M487FilterWmiApp.exe --query-ring        # Query ring buffer status
 *   M487FilterWmiApp.exe --query-config     # Query intercept config
 *   M487FilterWmiApp.exe --set-config --enable-set-feature
 *   M487FilterWmiApp.exe --monitor           # Continuous monitoring
 *
 * WMI Namespaces:
 *   - root\wmi (where M487Filter WMI classes are registered)
 *
 * WMI Classes:
 *   - M487FilterDeviceInformation  (GUID: C8E5F3B0-E8C4-4F7A-9A3D-7B2C4E1F5A9D)
 *   - M487FilterRingBufferInfo     (GUID: D4E5F6A7-8C9D-4B5A-B3C2-1E0F2A3B4C5D)
 *   - M487FilterInterceptConfigWmi (GUID: E5F6A7B8-9D8C-5A4B-C3D2-0F1E2A3B4C5D)
 *
 * Environment:
 *   Windows user-mode
 *
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <wbemcli.h>
#include <comdef.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

//
// WMI Provider GUIDs (must match driver)
//

// M487FilterDeviceInformation
static const GUID M487FilterDeviceInformation_GUID =
    { 0xC8E5F3B0, 0xE8C4, 0x4F7A, { 0x9A, 0x3D, 0x7B, 0x2C, 0x4E, 0x1F, 0x5A, 0x9D } };

// M487FilterRingBufferInfo
static const GUID M487FilterRingBufferInfo_GUID =
    { 0xD4E5F6A7, 0x8C9D, 0x4B5A, { 0xB3, 0xC2, 0x1E, 0x0F, 0x2A, 0x3B, 0x4C, 0x5D } };

// M487FilterInterceptConfigWmi
static const GUID M487FilterInterceptConfigWmi_GUID =
    { 0xE5F6A7B8, 0x9D8C, 0x5A4B, { 0xC3, 0xD2, 0x0F, 0x1E, 0x2A, 0x3B, 0x4C, 0x5D } };

//
// WMI Class Names
//
#define WMI_NAMESPACE         L"root\\wmi"
#define CLASS_DEVICE_INFO     L"M487FilterDeviceInformation"
#define CLASS_RING_BUFFER      L"M487FilterRingBufferInfo"
#define CLASS_INTERCEPT_CONFIG L"M487FilterInterceptConfigWmi"

//
// WMI Result Structures (mirror driver definitions)
//

typedef struct _M487FilterDeviceInformationWmi
{
    BOOLEAN Active;
    BOOLEAN PassThroughEnabled;
    BOOLEAN CaptureEnabled;

} M487FilterDeviceInformationWmi;

typedef struct _M487FilterRingBufferInfoWmi
{
    ULONGLONG RingBufferPhysicalAddress;
    ULONG     RingBufferSize;
    ULONGLONG TotalCaptured;
    ULONG     DroppedCount;
    ULONG     CurrentUsedSlots;
    ULONG     TotalSlots;
    ULONG     SlotSize;
    BOOLEAN   RingInitialized;

} M487FilterRingBufferInfoWmi;

typedef struct _M487FilterInterceptConfigWmi
{
    BOOLEAN CaptureSetFeature;
    BOOLEAN CaptureGetFeature;
    BOOLEAN CaptureWriteReport;
    BOOLEAN CaptureReadReport;

} M487FilterInterceptConfigWmi;

//
// Globals
//
static IWbemServices*     g_pSvc = NULL;
static IWbemLocator*      g_pLoc = NULL;
static BOOL                g_ComInitialized = FALSE;

//
// Function prototypes
//
static HRESULT InitWmi(void);
static void    ShutdownWmi(void);
static HRESULT ExecuteQueryWmi(LPCWSTR ClassName, IEnumWbemClassObject** ppEnum);
static HRESULT GetSingleInstance(LPCWSTR ClassName, IWbemClassObject** ppObj);

static HRESULT QueryRingBufferInfo(M487FilterRingBufferInfoWmi* pInfo);
static HRESULT QueryInterceptConfig(M487FilterInterceptConfigWmi* pConfig);
static HRESULT QueryDeviceInfo(M487FilterDeviceInformationWmi* pInfo);
static HRESULT SetInterceptConfig(const M487FilterInterceptConfigWmi* pConfig);
static HRESULT SetDeviceInfo(const M487FilterDeviceInformationWmi* pInfo);

static void    PrintRingBufferInfo(const M487FilterRingBufferInfoWmi* pInfo);
static void    PrintInterceptConfig(const M487FilterInterceptConfigWmi* pConfig);
static void    PrintDeviceInfo(const M487FilterDeviceInformationWmi* pInfo);

static LPCWSTR GuidToString(const GUID* pGuid);
static void   ParseCommandLine(int argc, WCHAR* argv[],
                               BOOL* pQueryRing, BOOL* pQueryConfig, BOOL* pQueryDevice,
                               BOOL* pSetConfig, BOOL* pSetDevice,
                               BOOL* pMonitor,
                               M487FilterInterceptConfigWmi* pNewConfig,
                               M487FilterDeviceInformationWmi* pNewDeviceInfo);

//
// InitWmi - Initialize COM and connect to WMI
//
static HRESULT InitWmi(void)
{
    HRESULT hr;

    //
    // Initialize COM
    //
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        wprintf(L"[ERROR] CoInitializeEx failed: 0x%08X\n", hr);
        return hr;
    }
    g_ComInitialized = TRUE;

    //
    // Set COM security level
    //
    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL);

    if (FAILED(hr)) {
        wprintf(L"[ERROR] CoInitializeSecurity failed: 0x%08X\n", hr);
        //
        // Not fatal on some systems - continue
        //
    }

    //
    // Create WMI Locator
    //
    hr = CoCreateInstance(
        &CLSID_WbemLocator,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        (LPVOID*)&g_pLoc);

    if (FAILED(hr)) {
        wprintf(L"[ERROR] CoCreateInstance(CLSID_WbemLocator) failed: 0x%08X\n", hr);
        return hr;
    }

    //
    // Connect to WMI namespace
    //
    hr = g_pLoc->lpVtbl->ConnectServer(
        g_pLoc,
        _bstr_t(WMI_NAMESPACE),   // namespace
        NULL,                      // User (current)
        NULL,                      // Locale
        0,                         // Security flags
        0,                         // Authority
        NULL,                      // Context
        &g_pSvc);                   // IWbemServices*

    if (FAILED(hr)) {
        wprintf(L"[ERROR] IWbemLocator::ConnectServer failed: 0x%08X\n", hr);
        wprintf(L"[ERROR] Make sure WMI is running and the driver is installed.\n");
        return hr;
    }

    wprintf(L"[INFO] Connected to WMI namespace: %s\n", WMI_NAMESPACE);

    //
    // Set proxy blanket for the services
    //
    hr = CoSetProxyBlanket(
        (IUnknown*)g_pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE);

    if (FAILED(hr)) {
        wprintf(L"[WARNING] CoSetProxyBlanket failed: 0x%08X\n", hr);
        //
        // Continue anyway - may still work
        //
    }

    return S_OK;
}

//
// ShutdownWmi - Release WMI resources
//
static void ShutdownWmi(void)
{
    if (g_pSvc) {
        g_pSvc->lpVtbl->Release(g_pSvc);
        g_pSvc = NULL;
    }
    if (g_pLoc) {
        g_pLoc->lpVtbl->Release(g_pLoc);
        g_pLoc = NULL;
    }
    if (g_ComInitialized) {
        CoUninitialize();
        g_ComInitialized = FALSE;
    }
}

//
// ExecuteQueryWmi - Execute a WQL query and return enumerator
//
static HRESULT ExecuteQueryWmi(LPCWSTR Query, IEnumWbemClassObject** ppEnum)
{
    HRESULT hr;

    if (!g_pSvc) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_HANDLE);
    }

    hr = g_pSvc->lpVtbl->ExecQuery(
        g_pSvc,
        _bstr_t(L"WQL"),
        _bstr_t(Query),
        WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
        NULL,
        ppEnum);

    if (FAILED(hr)) {
        wprintf(L"[ERROR] IWbemServices::ExecQuery failed: 0x%08X\n", hr);
    }

    return hr;
}

//
// GetSingleInstance - Get single instance of a class
//
static HRESULT GetSingleInstance(LPCWSTR ClassName, IWbemClassObject** ppObj)
{
    HRESULT hr;
    IEnumWbemClassObject* pEnum = NULL;
    IWbemClassObject* pObj = NULL;
    ULONG uReturned;

    wprintf(L"[INFO] Querying class: %s\n", ClassName);

    //
    // Build query: SELECT * FROM <ClassName>
    //
    WCHAR Query[256];
    _snwprintf(Query, sizeof(Query) / sizeof(WCHAR),
               L"SELECT * FROM %s", ClassName);

    hr = ExecuteQueryWmi(Query, &pEnum);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Get first instance
    //
    hr = pEnum->lpVtbl->Next(pEnum, WBEM_INFINITE, 1, &pObj, &uReturned);
    if (hr == WBEM_S_FALSE) {
        wprintf(L"[ERROR] No instances of %s found.\n", ClassName);
        wprintf(L"[INFO] Make sure the M487Filter driver is loaded and WMI provider is registered.\n");
        hr = WBEM_E_NOT_FOUND;
    } else if (FAILED(hr)) {
        wprintf(L"[ERROR] IEnumWbemClassObject::Next failed: 0x%08X\n", hr);
    } else {
        wprintf(L"[INFO] Found instance of %s\n", ClassName);
        *ppObj = pObj;  // Transfer reference
        hr = S_OK;
    }

    pEnum->lpVtbl->Release(pEnum);
    return hr;
}

//
// QueryRingBufferInfo - Query ring buffer status via WMI
//
static HRESULT QueryRingBufferInfo(M487FilterRingBufferInfoWmi* pInfo)
{
    HRESULT hr;
    IWbemClassObject* pObj = NULL;
    VARIANT vtProp;
    CIMTYPE cimType;

    if (!pInfo) return E_POINTER;

    ZeroMemory(pInfo, sizeof(*pInfo));

    hr = GetSingleInstance(CLASS_RING_BUFFER, &pObj);
    if (FAILED(hr)) {
        return hr;
    }

    //
    // Read all properties
    //

    // RingBufferPhysicalAddress (uint64)
    hr = pObj->lpVtbl->Get(pObj, L"RingBufferPhysicalAddress", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_UI8) {
        pInfo->RingBufferPhysicalAddress = vtProp.ullVal;
    }
    VariantClear(&vtProp);

    // RingBufferSize (uint32)
    hr = pObj->lpVtbl->Get(pObj, L"RingBufferSize", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_UI4) {
        pInfo->RingBufferSize = vtProp.ulVal;
    }
    VariantClear(&vtProp);

    // TotalCaptured (uint64)
    hr = pObj->lpVtbl->Get(pObj, L"TotalCaptured", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && (vtProp.vt == VT_UI8 || vtProp.vt == VT_I8)) {
        pInfo->TotalCaptured = vtProp.ullVal;
    }
    VariantClear(&vtProp);

    // DroppedCount (uint32)
    hr = pObj->lpVtbl->Get(pObj, L"DroppedCount", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_UI4) {
        pInfo->DroppedCount = vtProp.ulVal;
    }
    VariantClear(&vtProp);

    // CurrentUsedSlots (uint32)
    hr = pObj->lpVtbl->Get(pObj, L"CurrentUsedSlots", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_UI4) {
        pInfo->CurrentUsedSlots = vtProp.ulVal;
    }
    VariantClear(&vtProp);

    // TotalSlots (uint32)
    hr = pObj->lpVtbl->Get(pObj, L"TotalSlots", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_UI4) {
        pInfo->TotalSlots = vtProp.ulVal;
    }
    VariantClear(&vtProp);

    // SlotSize (uint32)
    hr = pObj->lpVtbl->Get(pObj, L"SlotSize", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_UI4) {
        pInfo->SlotSize = vtProp.ulVal;
    }
    VariantClear(&vtProp);

    // RingInitialized (boolean)
    hr = pObj->lpVtbl->Get(pObj, L"RingInitialized", 0,
                           &vtProp, &cimType, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pInfo->RingInitialized = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    pObj->lpVtbl->Release(pObj);
    return S_OK;
}

//
// QueryInterceptConfig - Query interception configuration via WMI
//
static HRESULT QueryInterceptConfig(M487FilterInterceptConfigWmi* pConfig)
{
    HRESULT hr;
    IWbemClassObject* pObj = NULL;
    VARIANT vtProp;

    if (!pConfig) return E_POINTER;

    ZeroMemory(pConfig, sizeof(*pConfig));

    hr = GetSingleInstance(CLASS_INTERCEPT_CONFIG, &pObj);
    if (FAILED(hr)) {
        return hr;
    }

    // CaptureSetFeature
    hr = pObj->lpVtbl->Get(pObj, L"CaptureSetFeature", 0,
                           &vtProp, NULL, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pConfig->CaptureSetFeature = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    // CaptureGetFeature
    hr = pObj->lpVtbl->Get(pObj, L"CaptureGetFeature", 0,
                           &vtProp, NULL, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pConfig->CaptureGetFeature = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    // CaptureWriteReport
    hr = pObj->lpVtbl->Get(pObj, L"CaptureWriteReport", 0,
                           &vtProp, NULL, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pConfig->CaptureWriteReport = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    // CaptureReadReport
    hr = pObj->lpVtbl->Get(pObj, L"CaptureReadReport", 0,
                           &vtProp, NULL, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pConfig->CaptureReadReport = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    pObj->lpVtbl->Release(pObj);
    return S_OK;
}

//
// QueryDeviceInfo - Query device information via WMI
//
static HRESULT QueryDeviceInfo(M487FilterDeviceInformationWmi* pInfo)
{
    HRESULT hr;
    IWbemClassObject* pObj = NULL;
    VARIANT vtProp;

    if (!pInfo) return E_POINTER;

    ZeroMemory(pInfo, sizeof(*pInfo));

    hr = GetSingleInstance(CLASS_DEVICE_INFO, &pObj);
    if (FAILED(hr)) {
        return hr;
    }

    // Active
    hr = pObj->lpVtbl->Get(pObj, L"Active", 0, &vtProp, NULL, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pInfo->Active = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    // PassThroughEnabled
    hr = pObj->lpVtbl->Get(pObj, L"PassThroughEnabled", 0, &vtProp, NULL, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pInfo->PassThroughEnabled = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    // CaptureEnabled
    hr = pObj->lpVtbl->Get(pObj, L"CaptureEnabled", 0, &vtProp, NULL, NULL);
    if (SUCCEEDED(hr) && vtProp.vt == VT_BOOL) {
        pInfo->CaptureEnabled = (vtProp.boolVal != VARIANT_FALSE);
    }
    VariantClear(&vtProp);

    pObj->lpVtbl->Release(pObj);
    return S_OK;
}

//
// SetInterceptConfig - Set interception configuration via WMI
//
static HRESULT SetInterceptConfig(const M487FilterInterceptConfigWmi* pConfig)
{
    HRESULT hr;
    IWbemClassObject* pObj = NULL;
    IWbemClassObject* pInParams = NULL;
    IWbemClassObject* pConfigClass = NULL;
    IWbemPath* pPath = NULL;
    BSTR strClass = NULL;
    BSTR strPath = NULL;
    BSTR strMethod = NULL;
    VARIANT vtSetFeature, vtGetFeature, vtWriteReport, vtReadReport;
    VARIANT vtConfig;
    CIMTYPE cimType;

    if (!pConfig) return E_POINTER;

    wprintf(L"[INFO] Setting Intercept Configuration via WMI...\n");
    wprintf(L"  CaptureSetFeature   = %s\n", pConfig->CaptureSetFeature ? L"TRUE" : L"FALSE");
    wprintf(L"  CaptureGetFeature   = %s\n", pConfig->CaptureGetFeature ? L"TRUE" : L"FALSE");
    wprintf(L"  CaptureWriteReport  = %s\n", pConfig->CaptureWriteReport ? L"TRUE" : L"FALSE");
    wprintf(L"  CaptureReadReport   = %s\n", pConfig->CaptureReadReport ? L"TRUE" : L"FALSE");

    //
    // Get the class definition
    //
    hr = g_pSvc->lpVtbl->GetObject(g_pSvc,
                                    _bstr_t(CLASS_INTERCEPT_CONFIG),
                                    0, NULL, &pConfigClass, NULL);
    if (FAILED(hr)) {
        wprintf(L"[ERROR] Failed to get class definition: 0x%08X\n", hr);
        return hr;
    }

    //
    // Get the instance path (need instance of the class first)
    //
    hr = GetSingleInstance(CLASS_INTERCEPT_CONFIG, &pObj);
    if (FAILED(hr)) {
        wprintf(L"[ERROR] Failed to get instance: 0x%08X\n", hr);
        pConfigClass->lpVtbl->Release(pConfigClass);
        return hr;
    }

    //
    // Build instance path from __PATH property
    //
    VARIANT vtPath;
    hr = pObj->lpVtbl->Get(pObj, L"__PATH", 0, &vtPath, NULL, NULL);
    if (FAILED(hr) || vtPath.vt != VT_BSTR) {
        wprintf(L"[ERROR] Failed to get __PATH: 0x%08X\n", hr);
        VariantClear(&vtPath);
        pObj->lpVtbl->Release(pObj);
        pConfigClass->lpVtbl->Release(pConfigClass);
        return hr;
    }
    strPath = vtPath.bstrVal;

    wprintf(L"[INFO] Instance path: %s\n", strPath);

    //
    // Use IWbemServices::PutInstance to set all properties at once
    // Clone the instance object, modify properties, then PutInstance
    //
    IWbemClassObject* pInstCopy = NULL;
    hr = pObj->lpVtbl->Clone(pObj, &pInstCopy);
    if (FAILED(hr)) {
        wprintf(L"[ERROR] Clone failed: 0x%08X\n", hr);
        VariantClear(&vtPath);
        pObj->lpVtbl->Release(pObj);
        pConfigClass->lpVtbl->Release(pConfigClass);
        return hr;
    }

    //
    // Set CaptureSetFeature
    //
    VariantInit(&vtSetFeature);
    vtSetFeature.vt = VT_BOOL;
    vtSetFeature.boolVal = pConfig->CaptureSetFeature ? VARIANT_TRUE : VARIANT_FALSE;
    hr = pInstCopy->lpVtbl->Put(pInstCopy, L"CaptureSetFeature", 0, &vtSetFeature, NULL);
    if (FAILED(hr)) {
        wprintf(L"[WARNING] Put CaptureSetFeature failed: 0x%08X\n", hr);
    }
    VariantClear(&vtSetFeature);

    //
    // Set CaptureGetFeature
    //
    VariantInit(&vtGetFeature);
    vtGetFeature.vt = VT_BOOL;
    vtGetFeature.boolVal = pConfig->CaptureGetFeature ? VARIANT_TRUE : VARIANT_FALSE;
    hr = pInstCopy->lpVtbl->Put(pInstCopy, L"CaptureGetFeature", 0, &vtGetFeature, NULL);
    if (FAILED(hr)) {
        wprintf(L"[WARNING] Put CaptureGetFeature failed: 0x%08X\n", hr);
    }
    VariantClear(&vtGetFeature);

    //
    // Set CaptureWriteReport
    //
    VariantInit(&vtWriteReport);
    vtWriteReport.vt = VT_BOOL;
    vtWriteReport.boolVal = pConfig->CaptureWriteReport ? VARIANT_TRUE : VARIANT_FALSE;
    hr = pInstCopy->lpVtbl->Put(pInstCopy, L"CaptureWriteReport", 0, &vtWriteReport, NULL);
    if (FAILED(hr)) {
        wprintf(L"[WARNING] Put CaptureWriteReport failed: 0x%08X\n", hr);
    }
    VariantClear(&vtWriteReport);

    //
    // Set CaptureReadReport
    //
    VariantInit(&vtReadReport);
    vtReadReport.vt = VT_BOOL;
    vtReadReport.boolVal = pConfig->CaptureReadReport ? VARIANT_TRUE : VARIANT_FALSE;
    hr = pInstCopy->lpVtbl->Put(pInstCopy, L"CaptureReadReport", 0, &vtReadReport, NULL);
    if (FAILED(hr)) {
        wprintf(L"[WARNING] Put CaptureReadReport failed: 0x%08X\n", hr);
    }
    VariantClear(&vtReadReport);

    //
    // Set the __PATH to the existing instance
    //
    hr = pInstCopy->lpVtbl->Put(pInstCopy, L"__PATH", 0, &vtPath, NULL);
    if (FAILED(hr)) {
        wprintf(L"[WARNING] Put __PATH failed: 0x%08X\n", hr);
    }

    //
    // Write the instance back to WMI
    //
    hr = g_pSvc->lpVtbl->PutInstance(g_pSvc, pInstCopy,
                                      WBEM_FLAG_UPDATE_ONLY, NULL, NULL);
    if (FAILED(hr)) {
        wprintf(L"[ERROR] PutInstance failed: 0x%08X\n", hr);
        wprintf(L"[INFO] Note: WMI SET may require elevated privileges or driver support.\n");
    } else {
        wprintf(L"[SUCCESS] Intercept configuration updated via WMI.\n");
    }

    VariantClear(&vtPath);
    pInstCopy->lpVtbl->Release(pInstCopy);
    pObj->lpVtbl->Release(pObj);
    pConfigClass->lpVtbl->Release(pConfigClass);

    return hr;
}

//
// PrintRingBufferInfo - Display ring buffer info
//
static void PrintRingBufferInfo(const M487FilterRingBufferInfoWmi* pInfo)
{
    if (!pInfo) return;

    wprintf(L"\n");
    wprintf(L"========================================\n");
    wprintf(L"       M487Filter Ring Buffer Info      \n");
    wprintf(L"========================================\n");
    wprintf(L"  Ring Initialized : %s\n", pInfo->RingInitialized ? L"YES" : L"NO");
    wprintf(L"  Physical Address : 0x%016I64X\n", pInfo->RingBufferPhysicalAddress);
    wprintf(L"  Buffer Size      : %lu bytes\n", pInfo->RingBufferSize);
    wprintf(L"  Total Slots      : %lu\n", pInfo->TotalSlots);
    wprintf(L"  Slot Size        : %lu bytes\n", pInfo->SlotSize);
    wprintf(L"  Used Slots       : %lu\n", pInfo->CurrentUsedSlots);
    wprintf(L"  Total Captured   : %I64u\n", pInfo->TotalCaptured);
    wprintf(L"  Dropped Count    : %lu\n", pInfo->DroppedCount);

    if (pInfo->TotalSlots > 0) {
        double usagePct = (double)pInfo->CurrentUsedSlots / (double)pInfo->TotalSlots * 100.0;
        wprintf(L"  Usage            : %.1f%%\n", usagePct);
    }
    wprintf(L"========================================\n");
    wprintf(L"\n");
}

//
// PrintInterceptConfig - Display intercept config
//
static void PrintInterceptConfig(const M487FilterInterceptConfigWmi* pConfig)
{
    if (!pConfig) return;

    wprintf(L"\n");
    wprintf(L"========================================\n");
    wprintf(L"    M487Filter Intercept Configuration   \n");
    wprintf(L"========================================\n");
    wprintf(L"  Capture SET_FEATURE  : %s\n", pConfig->CaptureSetFeature ? L"ENABLED " : L"disabled");
    wprintf(L"  Capture GET_FEATURE  : %s\n", pConfig->CaptureGetFeature ? L"ENABLED " : L"disabled");
    wprintf(L"  Capture WRITE_REPORT : %s\n", pConfig->CaptureWriteReport ? L"ENABLED " : L"disabled");
    wprintf(L"  Capture READ_REPORT  : %s\n", pConfig->CaptureReadReport ? L"ENABLED " : L"disabled");
    wprintf(L"========================================\n");
    wprintf(L"\n");
}

//
// PrintDeviceInfo - Display device info
//
static void PrintDeviceInfo(const M487FilterDeviceInformationWmi* pInfo)
{
    if (!pInfo) return;

    wprintf(L"\n");
    wprintf(L"========================================\n");
    wprintf(L"      M487Filter Device Information     \n");
    wprintf(L"========================================\n");
    wprintf(L"  Active           : %s\n", pInfo->Active ? L"YES" : L"NO");
    wprintf(L"  PassThrough Mode : %s\n", pInfo->PassThroughEnabled ? L"ENABLED " : L"disabled");
    wprintf(L"  Capture Enabled  : %s\n", pInfo->CaptureEnabled ? L"YES" : L"NO");
    wprintf(L"========================================\n");
    wprintf(L"\n");
}

//
// ParseCommandLine - Parse command line arguments
//
static void ParseCommandLine(int argc, WCHAR* argv[],
                            BOOL* pQueryRing, BOOL* pQueryConfig, BOOL* pQueryDevice,
                            BOOL* pSetConfig, BOOL* pSetDevice,
                            BOOL* pMonitor,
                            M487FilterInterceptConfigWmi* pNewConfig,
                            M487FilterDeviceInformationWmi* pNewDeviceInfo)
{
    int i;

    ZeroMemory(pNewConfig, sizeof(*pNewConfig));
    ZeroMemory(pNewDeviceInfo, sizeof(*pNewDeviceInfo));

    //
    // Default: query all
    //
    *pQueryRing = FALSE;
    *pQueryConfig = FALSE;
    *pQueryDevice = FALSE;
    *pSetConfig = FALSE;
    *pSetDevice = FALSE;
    *pMonitor = FALSE;

    for (i = 1; i < argc; i++) {
        WCHAR* arg = argv[i];

        if (_wcsicmp(arg, L"--query-ring") == 0 || _wcsicmp(arg, L"-qr") == 0) {
            *pQueryRing = TRUE;
        }
        else if (_wcsicmp(arg, L"--query-config") == 0 || _wcsicmp(arg, L"-qc") == 0) {
            *pQueryConfig = TRUE;
        }
        else if (_wcsicmp(arg, L"--query-device") == 0 || _wcsicmp(arg, L"-qd") == 0) {
            *pQueryDevice = TRUE;
        }
        else if (_wcsicmp(arg, L"--query-all") == 0 || _wcsicmp(arg, L"-qa") == 0) {
            *pQueryRing = TRUE;
            *pQueryConfig = TRUE;
            *pQueryDevice = TRUE;
        }
        else if (_wcsicmp(arg, L"--set-config") == 0 || _wcsicmp(arg, L"-sc") == 0) {
            *pSetConfig = TRUE;
            //
            // Next args set the config flags
            //
            while (i + 1 < argc && argv[i + 1][0] != L'-') {
                WCHAR* val = argv[++i];
                if (_wcsicmp(val, L"--enable-set-feature") == 0) {
                    pNewConfig->CaptureSetFeature = TRUE;
                } else if (_wcsicmp(val, L"--disable-set-feature") == 0) {
                    pNewConfig->CaptureSetFeature = FALSE;
                } else if (_wcsicmp(val, L"--enable-get-feature") == 0) {
                    pNewConfig->CaptureGetFeature = TRUE;
                } else if (_wcsicmp(val, L"--disable-get-feature") == 0) {
                    pNewConfig->CaptureGetFeature = FALSE;
                } else if (_wcsicmp(val, L"--enable-write-report") == 0) {
                    pNewConfig->CaptureWriteReport = TRUE;
                } else if (_wcsicmp(val, L"--disable-write-report") == 0) {
                    pNewConfig->CaptureWriteReport = FALSE;
                } else if (_wcsicmp(val, L"--enable-read-report") == 0) {
                    pNewConfig->CaptureReadReport = TRUE;
                } else if (_wcsicmp(val, L"--disable-read-report") == 0) {
                    pNewConfig->CaptureReadReport = FALSE;
                } else if (_wcsicmp(val, L"--all") == 0) {
                    pNewConfig->CaptureSetFeature = TRUE;
                    pNewConfig->CaptureGetFeature = TRUE;
                    pNewConfig->CaptureWriteReport = TRUE;
                    pNewConfig->CaptureReadReport = TRUE;
                } else if (_wcsicmp(val, L"--none") == 0) {
                    pNewConfig->CaptureSetFeature = FALSE;
                    pNewConfig->CaptureGetFeature = FALSE;
                    pNewConfig->CaptureWriteReport = FALSE;
                    pNewConfig->CaptureReadReport = FALSE;
                } else {
                    //
                    // Not a flag, push back
                    //
                    i--;
                    break;
                }
            }
        }
        else if (_wcsicmp(arg, L"--set-device") == 0 || _wcsicmp(arg, L"-sd") == 0) {
            *pSetDevice = TRUE;
            //
            // Next args set the device flags
            //
            while (i + 1 < argc && argv[i + 1][0] != L'-') {
                WCHAR* val = argv[++i];
                if (_wcsicmp(val, L"--passthrough-on") == 0) {
                    pNewDeviceInfo->PassThroughEnabled = TRUE;
                } else if (_wcsicmp(val, L"--passthrough-off") == 0) {
                    pNewDeviceInfo->PassThroughEnabled = FALSE;
                } else if (_wcsicmp(val, L"--capture-on") == 0) {
                    pNewDeviceInfo->CaptureEnabled = TRUE;
                } else if (_wcsicmp(val, L"--capture-off") == 0) {
                    pNewDeviceInfo->CaptureEnabled = FALSE;
                } else {
                    i--;
                    break;
                }
            }
        }
        else if (_wcsicmp(arg, L"--monitor") == 0 || _wcsicmp(arg, L"-m") == 0) {
            *pMonitor = TRUE;
        }
        else if (_wcsicmp(arg, L"--help") == 0 || _wcsicmp(arg, L"-h") == 0 ||
                 _wcsicmp(arg, L"/?") == 0) {
            wprintf(L"\nM487Filter WMI Query Application\n");
            wprintf(L"\nUsage: M487FilterWmiApp.exe [options]\n");
            wprintf(L"\nQuery Options:\n");
            wprintf(L"  --query-ring, -qr       Query ring buffer status\n");
            wprintf(L"  --query-config, -qc     Query intercept configuration\n");
            wprintf(L"  --query-device, -qd     Query device information\n");
            wprintf(L"  --query-all, -qa        Query all WMI classes (default)\n");
            wprintf(L"\nSet Options:\n");
            wprintf(L"  --set-config [flags]    Set intercept configuration\n");
            wprintf(L"    Flags:\n");
            wprintf(L"      --enable-set-feature\n");
            wprintf(L"      --disable-set-feature\n");
            wprintf(L"      --enable-get-feature\n");
            wprintf(L"      --disable-get-feature\n");
            wprintf(L"      --enable-write-report\n");
            wprintf(L"      --disable-write-report\n");
            wprintf(L"      --enable-read-report\n");
            wprintf(L"      --disable-read-report\n");
            wprintf(L"      --all (enable all)\n");
            wprintf(L"      --none (disable all)\n");
            wprintf(L"  --set-device [flags]    Set device information\n");
            wprintf(L"    Flags:\n");
            wprintf(L"      --passthrough-on / --passthrough-off\n");
            wprintf(L"      --capture-on / --capture-off\n");
            wprintf(L"\nMonitoring:\n");
            wprintf(L"  --monitor, -m           Monitor ring buffer continuously\n");
            wprintf(L"\nExamples:\n");
            wprintf(L"  M487FilterWmiApp.exe --query-all\n");
            wprintf(L"  M487FilterWmiApp.exe --query-ring\n");
            wprintf(L"  M487FilterWmiApp.exe --set-config --all\n");
            wprintf(L"  M487FilterWmiApp.exe --set-config --enable-set-feature --enable-get-feature\n");
            wprintf(L"  M487FilterWmiApp.exe --monitor\n");
            exit(0);
        }
        else {
            wprintf(L"[WARNING] Unknown option: %s (use --help)\n", arg);
        }
    }

    //
    // Default to --query-all if nothing specified
    //
    if (!*pQueryRing && !*pQueryConfig && !*pQueryDevice && !*pSetConfig && !*pSetDevice && !*pMonitor) {
        *pQueryRing = TRUE;
        *pQueryConfig = TRUE;
        *pQueryDevice = TRUE;
    }
}

//
// MonitorRingBuffer - Continuously monitor ring buffer
//
static void MonitorRingBuffer(DWORD IntervalMs)
{
    M487FilterRingBufferInfoWmi prevInfo;
    ULONGLONG prevCaptured = 0;
    BOOL first = TRUE;

    wprintf(L"[INFO] Monitoring ring buffer every %lu ms (Ctrl+C to stop)\n\n", IntervalMs);

    while (1) {
        M487FilterRingBufferInfoWmi info;
        HRESULT hr = QueryRingBufferInfo(&info);

        if (SUCCEEDED(hr)) {
            if (!first && info.TotalCaptured != prevCaptured) {
                ULONGLONG newEntries = info.TotalCaptured - prevCaptured;
                wprintf(L"[%I64u] +%I64u entries | Used: %4lu/%4lu | Dropped: %4lu | %s\n",
                        info.TotalCaptured,
                        newEntries,
                        info.CurrentUsedSlots,
                        info.TotalSlots,
                        info.DroppedCount,
                        info.RingInitialized ? L"OK" : L"NOT INIT");
            } else if (first) {
                PrintRingBufferInfo(&info);
            }

            prevCaptured = info.TotalCaptured;
            prevInfo = info;
            first = FALSE;
        } else {
            wprintf(L"[%lu] Query failed: 0x%08X\n", GetTickCount(), hr);
        }

        Sleep(IntervalMs);
    }
}

//
// wmain - Entry point
//
int wmain(int argc, WCHAR* argv[])
{
    HRESULT hr;
    BOOL queryRing, queryConfig, queryDevice, setConfig, setDevice, monitor;
    M487FilterInterceptConfigWmi newConfig;
    M487FilterDeviceInformationWmi newDeviceInfo;
    M487FilterRingBufferInfoWmi ringInfo;
    M487FilterInterceptConfigWmi config;
    M487FilterDeviceInformationWmi deviceInfo;

    wprintf(L"M487Filter WMI Query Application\n");
    wprintf(L"================================\n\n");

    ParseCommandLine(argc, argv,
                    &queryRing, &queryConfig, &queryDevice,
                    &setConfig, &setDevice, &monitor,
                    &newConfig, &newDeviceInfo);

    //
    // Initialize WMI
    //
    hr = InitWmi();
    if (FAILED(hr)) {
        wprintf(L"\n[ERROR] Failed to initialize WMI. Are you running as Administrator?\n");
        wprintf(L"[ERROR] WMI must be accessible to query driver status.\n");
        ShutdownWmi();
        return 1;
    }

    wprintf(L"\n");

    //
    // Handle monitoring mode
    //
    if (monitor) {
        MonitorRingBuffer(1000);  // 1 second interval
        ShutdownWmi();
        return 0;
    }

    //
    // Handle query operations
    //
    if (queryRing) {
        hr = QueryRingBufferInfo(&ringInfo);
        if (SUCCEEDED(hr)) {
            PrintRingBufferInfo(&ringInfo);
        } else {
            wprintf(L"[ERROR] QueryRingBufferInfo failed: 0x%08X\n", hr);
        }
    }

    if (queryConfig) {
        hr = QueryInterceptConfig(&config);
        if (SUCCEEDED(hr)) {
            PrintInterceptConfig(&config);
        } else {
            wprintf(L"[ERROR] QueryInterceptConfig failed: 0x%08X\n", hr);
        }
    }

    if (queryDevice) {
        hr = QueryDeviceInfo(&deviceInfo);
        if (SUCCEEDED(hr)) {
            PrintDeviceInfo(&deviceInfo);
        } else {
            wprintf(L"[ERROR] QueryDeviceInfo failed: 0x%08X\n", hr);
        }
    }

    //
    // Handle set operations
    //
    if (setConfig) {
        hr = SetInterceptConfig(&newConfig);
        if (FAILED(hr)) {
            wprintf(L"[ERROR] SetInterceptConfig failed: 0x%08X\n", hr);
        }
        //
        // Verify
        //
        wprintf(L"[INFO] Verifying configuration...\n");
        hr = QueryInterceptConfig(&config);
        if (SUCCEEDED(hr)) {
            PrintInterceptConfig(&config);
        }
    }

    if (setDevice) {
        //
        // Note: SetDeviceInfo requires implementing WMI PutInstance
        // For now, we use the IOCTL path or the InterceptConfig class
        //
        wprintf(L"[INFO] Device info set requested (implementation: use --set-config)\n");
    }

    ShutdownWmi();

    wprintf(L"[INFO] Done.\n");
    return 0;
}
