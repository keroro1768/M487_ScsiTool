/**
 * HID I2C Bridge - Command Line Interface
 * 
 * Tool for testing M487 HID I2C Bridge functionality
 */

#include "hid_i2c.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <conio.h>

void printBanner()
{
    std::cout << "\n";
    std::cout << "  ===========================================\n";
    std::cout << "       M487 HID I2C Bridge CLI Tool\n";
    std::cout << "  ===========================================\n";
    std::cout << "\n";
}

void printHelp()
{
    std::cout << "  Commands:\n";
    std::cout << "    enumerate, enum   - List available HID I2C Bridge devices\n";
    std::cout << "    connect           - Connect to first device\n";
    std::cout << "    disconnect        - Disconnect from device\n";
    std::cout << "    status            - Show connection status\n";
    std::cout << "    \n";
    std::cout << "  I2C Commands:\n";
    std::cout << "    scan              - I2C: Scan for devices (quick test)\n";
    std::cout << "    write <addr> <hex> - I2C: Write data to slave\n";
    std::cout << "    read <addr> <len> - I2C: Read bytes from slave\n";
    std::cout << "    writeread <addr> <whex> <rlen> - I2C: Write then read\n";
    std::cout << "    \n";
    std::cout << "  Examples:\n";
    std::cout << "    scan               - Scan all I2C addresses\n";
    std::cout << "    write 0x3C 0x00  - Write 0x00 to device at 0x3C\n";
    std::cout << "    read 0x3C 1       - Read 1 byte from 0x3C\n";
    std::cout << "    writeread 0x3C 0x00 1 - Write reg addr, read 1 byte\n";
    std::cout << "    \n";
    std::cout << "    help               - Show this help\n";
    std::cout << "    exit, quit        - Exit program\n";
    std::cout << "\n";
}

std::vector<uint8_t> parseHexString(const std::string& hexStr)
{
    std::vector<uint8_t> result;
    std::string cleaned;
    
    for (char c : hexStr) {
        if (c >= '0' && c <= '9') cleaned.push_back(c);
        else if (c >= 'A' && c <= 'F') cleaned.push_back(c);
        else if (c >= 'a' && c <= 'f') cleaned.push_back(c);
    }

    for (size_t i = 0; i + 1 < cleaned.size(); i += 2) {
        std::string byteStr = cleaned.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        result.push_back(byte);
    }

    return result;
}

std::string bytesToHex(const uint8_t* data, size_t length)
{
    std::ostringstream oss;
    for (size_t i = 0; i < length; ++i) {
        if (i > 0) {
            if (i % 16 == 0) oss << "\n    ";
            else if (i % 2 == 0) oss << " ";
        }
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(data[i]);
    }
    return oss.str();
}

std::string toHex(uint8_t val)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)val;
    return oss.str();
}

int main()
{
    setlocale(LC_ALL, "");

    HidI2CBridge bridge;
    printBanner();
    printHelp();

    while (true) {
        std::cout << "\nHID-I2C> ";
        std::cout.flush();

        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        // Trim whitespace
        while (!line.empty() && (line[0] == ' ' || line[0] == '\t')) {
            line = line.substr(1);
        }
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t')) {
            line.pop_back();
        }

        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        // Convert to lowercase
        for (char& c : cmd) {
            if (c >= 'A' && c <= 'Z') c = c + 32;
        }

        if (cmd == "exit" || cmd == "quit") {
            std::cout << "Goodbye!\n";
            break;
        }
        else if (cmd == "help") {
            printHelp();
        }
        else if (cmd == "enumerate" || cmd == "enum") {
            std::cout << "  Enumerating HID I2C Bridge devices (VID=0x0416, PID=0x5020)...\n";
            if (bridge.enumerateDevices()) {
                const auto& list = bridge.getDeviceList();
                for (int i = 0; i < static_cast<int>(list.size()); ++i) {
                    std::cout << "  [" << i << "] " << list[i].substr(0, 60) << "...\n";
                }
                std::cout << "  Found " << list.size() << " device(s)\n";
            } else {
                std::cout << "  No devices found. Is the device connected?\n";
            }
        }
        else if (cmd == "connect") {
            if (bridge.isConnected()) {
                std::cout << "  Already connected.\n";
                continue;
            }
            std::cout << "  Connecting...\n";
            if (bridge.connect(0)) {
                std::cout << "  [OK] Connected to HID I2C Bridge\n";
            } else {
                std::cout << "  [FAIL] " << bridge.getLastError() << "\n";
            }
        }
        else if (cmd == "disconnect") {
            bridge.disconnect();
            std::cout << "  Disconnected.\n";
        }
        else if (cmd == "status") {
            if (bridge.isConnected()) {
                std::cout << "  Status: Connected\n";
            } else {
                std::cout << "  Status: Not connected\n";
            }
        }
        else if (cmd == "scan") {
            if (!bridge.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            std::cout << "  Scanning I2C bus for devices...\n";
            std::cout << "       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n";
            
            for (int base = 0; base < 128; base += 16) {
                std::cout << "  " << std::setw(2) << std::setfill('0') << std::hex << base << "0: ";
                for (int offset = 0; offset < 16; ++offset) {
                    uint8_t addr = static_cast<uint8_t>(base + offset);
                    uint8_t dummy = 0;
                    
                    // Try to read 1 byte - if ACK, device exists
                    if (bridge.i2cRead(addr, &dummy, 1)) {
                        std::cout << std::setfill(' ') << std::setw(2) << std::hex << (int)addr << " ";
                    } else {
                        std::cout << "-- ";
                    }
                }
                std::cout << "\n";
            }
            std::cout << std::dec;
        }
        else if (cmd == "write") {
            if (!bridge.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint8_t addr = 0;
            std::string hexStr;
            iss >> std::hex >> addr >> hexStr;

            if (hexStr.empty()) {
                std::cout << "  Usage: write <addr> <hex_data>\n";
                std::cout << "  Example: write 0x3C 0x00123344\n";
                continue;
            }

            auto data = parseHexString(hexStr);
            if (data.empty()) {
                std::cout << "  Invalid hex data\n";
                continue;
            }

            std::cout << "  Writing " << data.size() << " bytes to 0x" << toHex(addr) << "...\n";
            if (bridge.i2cWrite(addr, data.data(), static_cast<uint16_t>(data.size()))) {
                std::cout << "  [OK] Write successful\n";
            } else {
                std::cout << "  [FAIL] " << bridge.getLastError() << "\n";
            }
        }
        else if (cmd == "read") {
            if (!bridge.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint8_t addr = 0;
            uint16_t len = 1;
            iss >> std::hex >> addr >> std::dec >> len;

            if (len == 0 || len > 62) len = 1;

            std::cout << "  Reading " << len << " bytes from 0x" << toHex(addr) << "...\n";
            
            std::vector<uint8_t> data(len);
            if (bridge.i2cRead(addr, data.data(), len)) {
                std::cout << "  [OK] Read successful\n";
                std::cout << "  Data (" << len << " bytes):" << bytesToHex(data.data(), len) << "\n";
            } else {
                std::cout << "  [FAIL] " << bridge.getLastError() << "\n";
            }
        }
        else if (cmd == "writeread") {
            if (!bridge.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint8_t addr = 0;
            std::string hexStr;
            uint16_t rlen = 1;
            iss >> std::hex >> addr >> hexStr >> std::dec >> rlen;

            if (hexStr.empty()) {
                std::cout << "  Usage: writeread <addr> <hex_write_data> <read_len>\n";
                std::cout << "  Example: writeread 0x3C 0x00 1\n";
                continue;
            }

            auto wdata = parseHexString(hexStr);
            if (wdata.empty()) {
                std::cout << "  Invalid hex data\n";
                continue;
            }

            if (rlen == 0 || rlen > 62) rlen = 1;

            std::cout << "  Write+Read: " << wdata.size() << " bytes -> " << rlen << " bytes from 0x" << toHex(addr) << "\n";
            
            std::vector<uint8_t> rdata(rlen);
            if (bridge.i2cWriteRead(addr, wdata.data(), static_cast<uint16_t>(wdata.size()), rdata.data(), rlen)) {
                std::cout << "  [OK] Write+Read successful\n";
                std::cout << "  Write Data: " << bytesToHex(wdata.data(), wdata.size()) << "\n";
                std::cout << "  Read Data:  " << bytesToHex(rdata.data(), rlen) << "\n";
            } else {
                std::cout << "  [FAIL] " << bridge.getLastError() << "\n";
            }
        }
        else {
            std::cout << "  Unknown command: " << cmd << "\n";
            std::cout << "  Type 'help' for available commands.\n";
        }
    }

    bridge.disconnect();
    return 0;
}
