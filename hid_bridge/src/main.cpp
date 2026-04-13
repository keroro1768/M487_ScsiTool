/**
 * @file     main.cpp
 * @brief    M487 USB HID I2C Bridge - Windows CLI Tool
 * @version  1.0.0
 * 
 * Targets: M487 USB Composite Device (VID=0x04F3, PID=0x0732)
 * 
 * Uses Windows HID API (hid.dll) with Interrupt Endpoint transfers.
 */

#include "hid_device.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>
#include <conio.h>

using namespace std;

/*---------------------------------------------------------------------------------------------------------*/
/* Helpers                                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
static void printBanner()
{
    cout << "\n";
    cout << "  ======================================================\n";
    cout << "        M487 USB HID I2C Bridge CLI Tool\n";
    cout << "        VID=0x04F3, PID=0x0732\n";
    cout << "  ======================================================\n";
    cout << "\n";
}

static void printHelp()
{
    cout << "  Device Commands:\n";
    cout << "    list                - List all M487 HID devices\n";
    cout << "    open [idx]          - Connect to device (default: index 0)\n";
    cout << "    close               - Disconnect from device\n";
    cout << "    info                - Show device info (VID/PID/version)\n";
    cout << "    test                - Test connection with device\n";
    cout << "\n";
    cout << "  I2C Commands:\n";
    cout << "    scan                - Scan I2C bus for devices\n";
    cout << "    write <addr> <hex>  - Write hex data to I2C slave\n";
    cout << "    read <addr> <len>   - Read <len> bytes from I2C slave\n";
    cout << "    writeread <addr> <whex> <rlen>\n";
    cout << "                       - Write hex data, then read <rlen> bytes\n";
    cout << "\n";
    cout << "  Examples:\n";
    cout << "    list                - Find all connected M487 devices\n";
    cout << "    open                - Connect to first device\n";
    cout << "    scan                - Scan I2C addresses 0x01-0x7F\n";
    cout << "    write 0x3C 0x00     - Write 0x00 to device at 0x3C\n";
    cout << "    read 0x3C 1         - Read 1 byte from 0x3C\n";
    cout << "    writeread 0x3C 0x00 1  - Write reg addr, read 1 byte\n";
    cout << "\n";
    cout << "    help, ?             - Show this help\n";
    cout << "    exit, quit          - Exit program\n";
    cout << "\n";
}

static vector<uint8_t> parseHexString(const string& hexStr)
{
    vector<uint8_t> result;
    string cleaned;

    for (char c : hexStr) {
        if (c >= '0' && c <= '9') cleaned += c;
        else if (c >= 'A' && c <= 'F') cleaned += c;
        else if (c >= 'a' && c <= 'f') cleaned += c;
        else if (c == ' ') continue; // ignore spaces
    }

    for (size_t i = 0; i + 1 < cleaned.size(); i += 2) {
        string byteStr = cleaned.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(stoi(byteStr, nullptr, 16));
        result.push_back(byte);
    }
    return result;
}

static string toHex(uint8_t val)
{
    ostringstream oss;
    oss << uppercase << hex << setw(2) << setfill('0') << (int)val;
    return oss.str();
}

static string bytesToHex(const uint8_t* data, size_t len, size_t cols = 16)
{
    ostringstream oss;
    for (size_t i = 0; i < len; ++i) {
        if (i > 0) {
            if (cols > 0 && i % cols == 0) oss << "\n    ";
            else if (i % 2 == 0) oss << " ";
        }
        oss << uppercase << hex << setw(2) << setfill('0') << (int)data[i];
    }
    return oss.str();
}

/*---------------------------------------------------------------------------------------------------------*/
/* Main                                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
int main()
{
    setlocale(LC_ALL, "");

    M487HidDevice device;
    printBanner();
    printHelp();

    while (true) {
        cout << "\nM487-HID> ";
        cout.flush();

        string line;
        if (!getline(cin, line)) break;

        // Trim
        size_t start = line.find_first_not_of(" \t");
        if (start == string::npos) continue;
        size_t end = line.find_last_not_of(" \t");
        line = line.substr(start, end - start + 1);
        if (line.empty()) continue;

        // Parse command
        istringstream iss(line);
        string cmd;
        iss >> cmd;

        // Lowercase cmd
        for (char& c : cmd) c = (c >= 'A' && c <= 'Z') ? (c + 32) : c;

        // ----------------------------------------------------------------
        // Exit
        // ----------------------------------------------------------------
        if (cmd == "exit" || cmd == "quit") {
            cout << "Goodbye!\n";
            break;
        }
        else if (cmd == "help" || cmd == "?") {
            printHelp();
        }
        // ----------------------------------------------------------------
        // list - enumerate devices
        // ----------------------------------------------------------------
        else if (cmd == "list") {
            cout << "  Searching for M487 HID devices (VID=0x04F3, PID=0x0732)...\n";

            if (!device.enumerate()) {
                cout << "  [INFO] No M487 HID devices found.\n";
                cout << "  Make sure the device is connected and the HID interface is active.\n";
                continue;
            }

            const auto& devs = device.getDevices();
            cout << "  Found " << devs.size() << " device(s):\n";
            for (size_t i = 0; i < devs.size(); ++i) {
                const auto& d = devs[i];
                cout << "  [" << i << "] " << d.description << "\n";
                cout << "      VID=0x" << toHex((uint8_t)(d.vid >> 8))
                     << toHex((uint8_t)(d.vid & 0xFF))
                     << " PID=0x" << toHex((uint8_t)(d.pid >> 8))
                     << toHex((uint8_t)(d.pid & 0xFF)) << "\n";
            }
        }
        // ----------------------------------------------------------------
        // open [idx] - connect
        // ----------------------------------------------------------------
        else if (cmd == "open") {
            size_t idx = 0;
            string idxStr;
            iss >> idxStr;
            if (!idxStr.empty()) {
                try { idx = stoul(idxStr); } catch (...) {}
            }

            if (device.isConnected()) {
                cout << "  Already connected. Use 'close' first.\n";
                continue;
            }

            // Ensure device list is populated
            if (device.deviceCount() == 0) {
                cout << "  Searching for devices...\n";
                device.enumerate();
            }

            cout << "  Connecting to device[" << idx << "]...\n";
            if (device.connect(idx)) {
                cout << "  [OK] Connected successfully.\n";

                HIDD_ATTRIBUTES attr = {};
                if (device.getDeviceAttributes(&attr)) {
                    cout << "      VID=0x" << toHex((uint8_t)(attr.VendorID >> 8))
                         << toHex((uint8_t)(attr.VendorID & 0xFF))
                         << " PID=0x" << toHex((uint8_t)(attr.ProductID >> 8))
                         << toHex((uint8_t)(attr.ProductID & 0xFF))
                         << " Ver=" << attr.VersionNumber << "\n";
                }
            } else {
                cout << "  [FAIL] " << device.getLastError() << "\n";
            }
        }
        // ----------------------------------------------------------------
        // close - disconnect
        // ----------------------------------------------------------------
        else if (cmd == "close") {
            device.disconnect();
            cout << "  Disconnected.\n";
        }
        // ----------------------------------------------------------------
        // info - show connection info
        // ----------------------------------------------------------------
        else if (cmd == "info") {
            if (!device.isConnected()) {
                cout << "  Not connected. Use 'open' first.\n";
                continue;
            }

            HIDD_ATTRIBUTES attr = {};
            if (device.getDeviceAttributes(&attr)) {
                cout << "  Device Info:\n";
                cout << "    VID:       0x" << toHex((uint8_t)(attr.VendorID >> 8))
                     << toHex((uint8_t)(attr.VendorID & 0xFF)) << "\n";
                cout << "    PID:       0x" << toHex((uint8_t)(attr.ProductID >> 8))
                     << toHex((uint8_t)(attr.ProductID & 0xFF)) << "\n";
                cout << "    Version:   " << attr.VersionNumber << "\n";
            } else {
                cout << "  [FAIL] Could not read device attributes.\n";
            }
        }
        // ----------------------------------------------------------------
        // test - test connection
        // ----------------------------------------------------------------
        else if (cmd == "test") {
            if (!device.isConnected()) {
                cout << "  Not connected. Use 'open' first.\n";
                continue;
            }

            cout << "  Testing connection...\n";
            if (device.testConnection()) {
                cout << "  [OK] Connection test passed.\n";
            } else {
                cout << "  [FAIL] " << device.getLastError() << "\n";
            }
        }
        // ----------------------------------------------------------------
        // scan - I2C bus scan
        // ----------------------------------------------------------------
        else if (cmd == "scan") {
            if (!device.isConnected()) {
                cout << "  Not connected. Use 'open' first.\n";
                continue;
            }

            cout << "  Scanning I2C bus...\n";
            cout << "       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n";

            uint8_t found[8] = {0};
            int nFound = device.i2cScan(found);

            for (int base = 0; base < 128; base += 16) {
                cout << "  " << setw(2) << setfill('0') << hex << base << "0: ";
                for (int offset = 0; offset < 16; ++offset) {
                    uint8_t addr = static_cast<uint8_t>(base + offset);

                    bool isFound = false;
                    for (int i = 0; i < nFound; ++i) {
                        if (found[i] == addr) { isFound = true; break; }
                    }

                    if (isFound) {
                        cout << setfill(' ') << setw(2) << hex << (int)addr << " ";
                    } else {
                        cout << "-- ";
                    }
                }
                cout << "\n";
            }
            cout << dec;

            if (nFound > 0) {
                cout << "  Found " << nFound << " device(s): ";
                for (int i = 0; i < nFound; ++i) {
                    cout << "0x" << toHex(found[i]);
                    if (i < nFound - 1) cout << ", ";
                }
                cout << "\n";
            } else {
                cout << "  No I2C devices found on the bus.\n";
            }
        }
        // ----------------------------------------------------------------
        // write <addr> <hex>
        // ----------------------------------------------------------------
        else if (cmd == "write") {
            if (!device.isConnected()) {
                cout << "  Not connected. Use 'open' first.\n";
                continue;
            }

            uint32_t addr = 0;
            string hexStr;
            iss >> hex >> addr >> hexStr;

            if (addr > 127) {
                cout << "  Invalid I2C address (0x01-0x7F)\n";
                continue;
            }
            if (hexStr.empty()) {
                cout << "  Usage: write <addr> <hex_data>\n";
                cout << "  Example: write 0x3C 0x00123344\n";
                continue;
            }

            auto data = parseHexString(hexStr);
            if (data.empty()) {
                cout << "  Invalid hex data.\n";
                continue;
            }

            cout << "  Writing " << data.size() << " byte(s) to 0x" << toHex((uint8_t)addr) << "...\n";
            if (device.i2cWrite((uint8_t)addr, data.data(), (uint16_t)data.size())) {
                cout << "  [OK] Write successful.\n";
            } else {
                cout << "  [FAIL] " << device.getLastError() << "\n";
            }
        }
        // ----------------------------------------------------------------
        // read <addr> <len>
        // ----------------------------------------------------------------
        else if (cmd == "read") {
            if (!device.isConnected()) {
                cout << "  Not connected. Use 'open' first.\n";
                continue;
            }

            uint32_t addr = 0, len = 1;
            iss >> hex >> addr >> dec >> len;

            if (addr > 127) {
                cout << "  Invalid I2C address (0x01-0x7F)\n";
                continue;
            }
            if (len == 0 || len > 62) len = 1;

            cout << "  Reading " << len << " byte(s) from 0x" << toHex((uint8_t)addr) << "...\n";

            vector<uint8_t> data(len, 0);
            if (device.i2cRead((uint8_t)addr, data.data(), (uint16_t)len)) {
                cout << "  [OK] Read successful.\n";
                cout << "  Data (" << len << " bytes):" << bytesToHex(data.data(), len) << "\n";
            } else {
                cout << "  [FAIL] " << device.getLastError() << "\n";
            }
        }
        // ----------------------------------------------------------------
        // writeread <addr> <whex> <rlen>
        // ----------------------------------------------------------------
        else if (cmd == "writeread") {
            if (!device.isConnected()) {
                cout << "  Not connected. Use 'open' first.\n";
                continue;
            }

            uint32_t addr = 0, rlen = 1;
            string hexStr;
            iss >> hex >> addr >> hexStr >> dec >> rlen;

            if (addr > 127) {
                cout << "  Invalid I2C address (0x01-0x7F)\n";
                continue;
            }
            if (hexStr.empty()) {
                cout << "  Usage: writeread <addr> <write_hex> <read_len>\n";
                cout << "  Example: writeread 0x3C 0x00 1\n";
                continue;
            }
            if (rlen == 0 || rlen > 62) rlen = 1;

            auto wdata = parseHexString(hexStr);
            if (wdata.empty()) {
                cout << "  Invalid hex data.\n";
                continue;
            }

            cout << "  Write+Read: " << wdata.size() << " bytes -> "
                 << rlen << " bytes from 0x" << toHex((uint8_t)addr) << "\n";

            vector<uint8_t> rdata(rlen, 0);
            if (device.i2cWriteRead((uint8_t)addr, wdata.data(), (uint16_t)wdata.size(),
                                    rdata.data(), (uint16_t)rlen)) {
                cout << "  [OK] Write+Read successful.\n";
                cout << "  Write Data: " << bytesToHex(wdata.data(), wdata.size()) << "\n";
                cout << "  Read Data:  " << bytesToHex(rdata.data(), rlen) << "\n";
            } else {
                cout << "  [FAIL] " << device.getLastError() << "\n";
            }
        }
        // ----------------------------------------------------------------
        // Unknown
        // ----------------------------------------------------------------
        else {
            cout << "  Unknown command: " << cmd << "\n";
            cout << "  Type 'help' for available commands.\n";
        }
    }

    device.disconnect();
    return 0;
}
