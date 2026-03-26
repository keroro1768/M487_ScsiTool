#include "device.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <chrono>
#include <conio.h>

void printBanner()
{
    std::cout << "\n";
    std::cout << "  ==========================================\n";
    std::cout << "       M487 USB Storage CLI Tool\n";
    std::cout << "       (Win32 C++ / WinUSB)\n";
    std::cout << "  ==========================================\n";
    std::cout << "\n";
}

void printHelp()
{
    std::cout << "  Commands:\n";
    std::cout << "    enumerate, enum       - List available M487 devices\n";
    std::cout << "    connect               - Connect to selected device\n";
    std::cout << "    disconnect            - Disconnect from device\n";
    std::cout << "    info                  - Show device info (capacity, etc.)\n";
    std::cout << "    inquiry               - SCSI INQUIRY\n";
    std::cout << "    read <lba> [count]    - SCSI READ (default count=1)\n";
    std::cout << "    write <lba> <hex>     - SCSI WRITE pattern (e.g., 0x55AA)\n";
    std::cout << "    dump <lba> [count]    - Hex dump sectors\n";
    std::cout << "    fill <lba> <count> <hex> - Fill sectors with pattern\n";
    std::cout << "    \n";
    std::cout << "  I2C Commands:\n";
    std::cout << "    i2c-info              - I2C: Read device string via vendor cmd\n";
    std::cout << "    i2c-write <addr> <hex> - I2C: Write data to slave\n";
    std::cout << "    i2c-read <addr> <len> - I2C: Read bytes from slave\n";
    std::cout << "    i2c-writeread <addr> <whex> <rlen> - I2C: Write then read\n";
    std::cout << "    \n";
    std::cout << "  Vendor Commands:\n";
    std::cout << "    vendor                - Vendor command: Read device string\n";
    std::cout << "    vendor-verify <size>  - Test byte-swap functionality\n";
    std::cout << "    \n";
    std::cout << "  Speed Test:\n";
    std::cout << "    speed-read [sectors]  - Test SCSI READ speed\n";
    std::cout << "    speed-write [sectors] - Test SCSI WRITE speed\n";
    std::cout << "    \n";
    std::cout << "    help                  - Show this help\n";
    std::cout << "    exit, quit            - Exit program\n";
    std::cout << "\n";
}

std::vector<uint8_t> parseHexString(const std::string& hexStr)
{
    std::vector<uint8_t> result;
    std::string cleaned;
    
    // Remove 0x prefix and spaces
    for (char c : hexStr) {
        if (c == '0' && (cleaned.empty() || cleaned.back() == 'x' || cleaned.back() == 'X')) {
            cleaned.push_back(c);
        } else if (c >= '0' && c <= '9') {
            cleaned.push_back(c);
        } else if (c >= 'A' && c <= 'F') {
            cleaned.push_back(c);
        } else if (c >= 'a' && c <= 'f') {
            cleaned.push_back(c);
        }
    }

    // Remove 0x prefix if present
    if (cleaned.size() >= 2 && cleaned[0] == '0' && (cleaned[1] == 'x' || cleaned[1] == 'X')) {
        cleaned = cleaned.substr(2);
    }

    // Parse as hex bytes
    for (size_t i = 0; i + 1 < cleaned.size(); i += 2) {
        std::string byteStr = cleaned.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        result.push_back(byte);
    }

    // If odd number of chars, add leading zero
    if (cleaned.size() % 2 == 1) {
        std::string byteStr = "0" + cleaned.substr(cleaned.size() - 1);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byteStr, nullptr, 16));
        result.insert(result.begin(), byte);
    }

    return result;
}

std::string toHex(uint8_t val)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << (int)val;
    return oss.str();
}

std::string bytesToHex(const uint8_t* data, size_t length, size_t maxBytes = 256)
{
    std::ostringstream oss;
    size_t showBytes = length < maxBytes ? length : maxBytes;
    
    for (size_t i = 0; i < showBytes; ++i)
    {
        if (i > 0) {
            if (i % 16 == 0) oss << "\n    ";
            else if (i % 2 == 0) oss << " ";
        }
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(data[i]);
    }
    
    if (length > maxBytes) {
        oss << "\n    ... (" << std::dec << (length - maxBytes) << " more bytes)";
    }
    
    return oss.str();
}

void printScsiResult(const std::string& operation, const ScsiResult& result)
{
    if (result.success) {
        std::cout << "  [OK] " << operation << " succeeded";
        if (!result.data.empty()) {
            std::cout << " (" << result.data.size() << " bytes)";
        }
        std::cout << "\n";
    } else {
        std::cout << "  [FAIL] " << operation << " failed";
        if (!result.errorMessage.empty()) {
            std::cout << ": " << result.errorMessage;
        }
        std::cout << " (CSW status: 0x" << std::hex << (int)result.cswStatus << ")\n";
    }
}

int main()
{
    setlocale(LC_ALL, "");

    M487Device device;
    int selectedDevice = 0;

    printBanner();
    printHelp();

    while (true) {
        std::cout << "\nM487> ";
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

        // Convert to lowercase for comparison
        std::string cmdLower = cmd;
        for (char& c : cmdLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (cmdLower == "exit" || cmdLower == "quit") {
            std::cout << "Goodbye!\n";
            break;
        }
        else if (cmdLower == "help") {
            printHelp();
        }
        else if (cmdLower == "enumerate" || cmdLower == "enum") {
            std::cout << "  Enumerating M487 USB Storage devices (VID=0x0416, PID=0x501E)...\n";
            if (device.enumerateDevices()) {
                const auto& list = device.getDeviceList();
                for (int i = 0; i < static_cast<int>(list.size()); ++i) {
                    std::cout << "  [" << i << "] " << list[i].description 
                              << " (VID=0x" << std::hex << list[i].vid 
                              << ", PID=0x" << list[i].pid << ")\n";
                }
                if (list.empty()) {
                    std::cout << "  No devices found. Is the device connected?\n";
                }
            } else {
                std::cout << "  Failed to enumerate devices.\n";
            }
        }
        else if (cmdLower == "connect") {
            if (device.isConnected()) {
                std::cout << "  Already connected.\n";
                continue;
            }
            std::cout << "  Connecting...\n";
            if (device.connect()) {
                std::cout << "  [OK] Connected to M487 USB Storage\n";
            } else {
                std::cout << "  [FAIL] Connection failed\n";
            }
        }
        else if (cmdLower == "disconnect") {
            device.disconnect();
            std::cout << "  Disconnected.\n";
        }
        else if (cmdLower == "info") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            DeviceInfo info;
            if (device.getDeviceInfo(info)) {
                std::cout << "  Device Information:\n";
                std::cout << "    VID:        " << info.vendorId << "\n";
                std::cout << "    PID:        " << info.productId << "\n";
                std::cout << "    Serial:     " << info.serialNumber << "\n";
                std::cout << "    Firmware:   " << info.firmwareVersion << "\n";
                std::cout << "    Total Sectors: " << std::dec << info.totalSectors << "\n";
                std::cout << "    Sector Size:   " << info.sectorSize << " bytes\n";
                std::cout << "    Total Capacity: " << std::dec 
                          << (static_cast<uint64_t>(info.totalSectors) * info.sectorSize / 1024) 
                          << " KB\n";
            } else {
                std::cout << "  Failed to get device info\n";
            }
        }
        else if (cmdLower == "inquiry") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            auto result = device.scsiInquiry();
            printScsiResult("INQUIRY", result);
            if (result.success && result.data.size() >= 36) {
                const auto* inq = reinterpret_cast<const SCSI_INQUIRY_DATA*>(result.data.data());
                std::cout << "    Vendor:     ";
                for (int i = 0; i < 8; ++i) std::cout << static_cast<char>(inq->vendorID[i]);
                std::cout << "\n";
                std::cout << "    Product:    ";
                for (int i = 0; i < 16; ++i) std::cout << static_cast<char>(inq->productID[i]);
                std::cout << "\n";
                std::cout << "    Revision:   ";
                for (int i = 0; i < 4; ++i) std::cout << static_cast<char>(inq->productRevision[i]);
                std::cout << "\n";
            }
        }
        else if (cmdLower == "read") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint32_t lba = 0;
            uint16_t count = 1;
            iss >> lba >> count;
            if (count == 0) count = 1;

            auto start = std::chrono::high_resolution_clock::now();
            auto result = device.scsiRead10(lba, count);
            auto end = std::chrono::high_resolution_clock::now();

            printScsiResult("READ10 (LBA=" + std::to_string(lba) + ", count=" + std::to_string(count) + ")", result);

            if (result.success && !result.data.empty()) {
                std::cout << "  Data (" << result.data.size() << " bytes):" 
                          << bytesToHex(result.data.data(), result.data.size(), 128) << "\n";

                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                if (duration > 0) {
                    double throughput = (result.data.size() * 1000.0) / (duration * 1024.0);
                    std::cout << "  Time: " << duration << " ms, Throughput: " 
                              << std::fixed << std::setprecision(2) << throughput << " KB/s\n";
                }
            }
        }
        else if (cmdLower == "write") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint32_t lba = 0;
            std::string hexStr;
            iss >> lba >> hexStr;

            if (hexStr.empty()) {
                std::cout << "  Usage: write <lba> <hex_pattern>\n";
                continue;
            }

            auto pattern = parseHexString(hexStr);
            if (pattern.empty()) {
                std::cout << "  Invalid hex pattern\n";
                continue;
            }

            uint32_t sectorSize = device.getSectorSize();
            std::vector<uint8_t> writeData(sectorSize);
            for (size_t i = 0; i < sectorSize; ++i) {
                writeData[i] = pattern[i % pattern.size()];
            }

            auto result = device.scsiWrite10(lba, 1, writeData.data());
            printScsiResult("WRITE10 (LBA=" + std::to_string(lba) + ")", result);
        }
        else if (cmdLower == "vendor") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            auto result = device.vendorReadString(512);
            printScsiResult("VENDOR READ STRING", result);
            if (result.success && !result.data.empty()) {
                std::string str(reinterpret_cast<const char*>(result.data.data()), result.data.size());
                // Trim null terminators
                size_t end = str.find('\0');
                if (end != std::string::npos) str.resize(end);
                                std::cout << "  Device String: \"" << str << "\"\n";
                std::cout << "  Expected:     \"ELAN-USB-I2C-BRIDGE-V0.1\"\n";
                std::cout << "  Hex: " << bytesToHex(result.data.data(), result.data.size()) << "\n";
            }
        }
        else if (cmdLower == "vendor-verify") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            size_t bufferSize = 512;
            iss >> bufferSize;

            // Create pattern: 0x55 0xAA repeating
            std::vector<uint8_t> original(bufferSize);
            for (size_t i = 0; i < bufferSize; ++i) {
                original[i] = (i % 2 == 0) ? 0x55 : 0xAA;
            }

            // Send vendor command (device should byte-swap and echo back)
            // Note: This is a placeholder - actual implementation depends on firmware
            auto result = device.vendorReadString(static_cast<uint16_t>(bufferSize));
            printScsiResult("VENDOR VERIFY (byte-swap test)", result);

            if (result.success && !result.data.empty()) {
                // Check if bytes are byte-swapped
                std::vector<uint8_t> swapped = original;
                ScsiCommands::byteSwapBuffer(swapped.data(), swapped.size());

                bool matchesSwapped = (result.data.size() == swapped.size()) &&
                    std::equal(result.data.begin(), result.data.end(), swapped.begin());
                bool matchesOriginal = (result.data.size() == original.size()) &&
                    std::equal(result.data.begin(), result.data.end(), original.begin());

                if (matchesSwapped) {
                    std::cout << "  ✓ Data is byte-swapped (0x55 0xAA -> 0xAA 0x55) - CORRECT\n";
                } else if (matchesOriginal) {
                    std::cout << "  ✗ Data matches original pattern - NO byte-swap detected\n";
                } else {
                    std::cout << "  ? Data differs from both patterns (possible firmware issue)\n";
                    std::cout << "  Expected (swapped): " << bytesToHex(swapped.data(), swapped.size(), 32) << "\n";
                    std::cout << "  Received:          " << bytesToHex(result.data.data(), result.data.size(), 32) << "\n";
                }
            }
        }
        else if (cmdLower == "dump") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint32_t lba = 0;
            uint16_t count = 1;
            iss >> lba >> count;
            if (count == 0) count = 1;

            std::cout << "  Dumping " << count << " sector(s) from LBA " << lba << "...\n";
            auto result = device.scsiRead10(lba, count);
            printScsiResult("READ", result);

            if (result.success && !result.data.empty()) {
                size_t offset = 0;
                size_t sectorSize = device.getSectorSize();
                for (uint16_t s = 0; s < count; ++s) {
                    std::cout << "\n  === Sector " << std::dec << (lba + s) << " ===\n";
                    std::cout << bytesToHex(result.data.data() + offset, sectorSize, sectorSize) << "\n";
                    offset += sectorSize;
                    if (offset >= result.data.size()) break;
                }
            }
        }
        else if (cmdLower == "fill") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint32_t lba = 0;
            uint16_t count = 1;
            std::string hexStr;
            iss >> lba >> count >> hexStr;

            if (hexStr.empty()) {
                std::cout << "  Usage: fill <lba> <count> <hex_pattern>\n";
                continue;
            }

            auto pattern = parseHexString(hexStr);
            if (pattern.empty()) {
                std::cout << "  Invalid hex pattern\n";
                continue;
            }

            uint32_t sectorSize = device.getSectorSize();
            std::vector<uint8_t> writeData(sectorSize * count);

            for (size_t i = 0; i < writeData.size(); ++i) {
                writeData[i] = pattern[i % pattern.size()];
            }

            auto result = device.scsiWrite10(lba, count, writeData.data());
            printScsiResult("FILL (LBA=" + std::to_string(lba) + ", count=" + std::to_string(count) + ")", result);
        }
        else if (cmdLower == "i2c-info") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            auto result = device.vendorReadString(512);
            printScsiResult("I2C INFO (vendor 0xC0 sub-cmd 0x00)", result);
            if (result.success && !result.data.empty()) {
                std::string str(reinterpret_cast<const char*>(result.data.data()), result.data.size());
                size_t end = str.find('\0');
                if (end != std::string::npos) str.resize(end);
                std::cout << "  Device String: \"" << str << "\"\n";
                std::cout << "  Expected:      \"ELAN-USB-I2C-BRIDGE-V0.1\"\n";
            }
        }
        else if (cmdLower == "i2c-write") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint8_t addr = 0;
            std::string hexStr;
            iss >> std::hex >> addr >> hexStr;

            if (hexStr.empty()) {
                std::cout << "  Usage: i2c-write <addr> <hex_data>\n";
                std::cout << "  Example: i2c-write 0x3C 0x00011234\n";
                continue;
            }

            auto data = parseHexString(hexStr);
            if (data.empty()) {
                std::cout << "  Invalid hex data\n";
                continue;
            }

            auto result = device.i2cWrite(addr, data.data(), static_cast<uint16_t>(data.size()));
            printScsiResult("I2C WRITE (addr=0x" + toHex(addr) + ", len=" + std::to_string(data.size()) + ")", result);
        }
        else if (cmdLower == "i2c-read") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint8_t addr = 0;
            uint16_t len = 1;
            iss >> std::hex >> addr >> std::dec >> len;

            if (len == 0 || len > 64) len = 64;

            auto result = device.i2cRead(addr, len);
            printScsiResult("I2C READ (addr=0x" + toHex(addr) + ", len=" + std::to_string(len) + ")", result);
            if (result.success && !result.data.empty()) {
                // Skip first byte (status)
                std::cout << "  Data (" << (result.data.size() - 1) << " bytes):";
                if (result.data.size() > 1) {
                    std::cout << bytesToHex(result.data.data() + 1, result.data.size() - 1) << "\n";
                }
            }
        }
        else if (cmdLower == "i2c-writeread") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint8_t addr = 0;
            std::string hexStr;
            uint16_t rlen = 1;
            iss >> std::hex >> addr >> hexStr >> std::dec >> rlen;

            if (hexStr.empty()) {
                std::cout << "  Usage: i2c-writeread <addr> <hex_write_data> <read_len>\n";
                std::cout << "  Example: i2c-writeread 0x3C 0x00 1\n";
                continue;
            }

            auto wdata = parseHexString(hexStr);
            if (wdata.empty()) {
                std::cout << "  Invalid hex data\n";
                continue;
            }

            if (rlen == 0 || rlen > 64) rlen = 64;

            auto result = device.i2cWriteRead(addr, wdata.data(), static_cast<uint16_t>(wdata.size()), rlen);
            printScsiResult("I2C WRITE+READ (addr=0x" + toHex(addr) + ", wlen=" + std::to_string(wdata.size()) + ", rlen=" + std::to_string(rlen) + ")", result);
            if (result.success && !result.data.empty()) {
                std::cout << "  Data (" << (result.data.size() - 1) << " bytes):";
                if (result.data.size() > 1) {
                    std::cout << bytesToHex(result.data.data() + 1, result.data.size() - 1) << "\n";
                }
            }
        }
        else if (cmdLower == "speed-read") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint16_t sectors = 128; // Default: 128 sectors = 64KB
            iss >> sectors;
            if (sectors == 0) sectors = 1;
            if (sectors > 65535) sectors = 65535;

            std::cout << "  Testing READ speed with " << sectors << " sectors (" << (sectors * 512 / 1024) << " KB)...\n";

            auto start = std::chrono::high_resolution_clock::now();
            auto result = device.scsiRead10(0, sectors);
            auto end = std::chrono::high_resolution_clock::now();

            printScsiResult("READ " + std::to_string(sectors) + " sectors", result);

            if (result.success) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                if (duration > 0) {
                    double throughputKBps = (result.data.size() * 1000.0) / (duration * 1024.0);
                    double throughputMBps = throughputKBps / 1024.0;
                    std::cout << "  " << std::fixed << std::setprecision(2) 
                              << result.data.size() << " bytes in " << duration << " ms\n";
                    std::cout << "  Throughput: " << throughputKBps << " KB/s (" 
                              << throughputMBps << " MB/s)\n";
                }
            }
        }
        else if (cmdLower == "speed-write") {
            if (!device.isConnected()) {
                std::cout << "  Not connected. Use 'connect' first.\n";
                continue;
            }
            uint16_t sectors = 128;
            iss >> sectors;
            if (sectors == 0) sectors = 1;
            if (sectors > 65535) sectors = 65535;

            std::cout << "  Testing WRITE speed with " << sectors << " sectors (" << (sectors * 512 / 1024) << " KB)...\n";

            // Create test pattern
            std::vector<uint8_t> writeData(sectors * device.getSectorSize());
            for (size_t i = 0; i < writeData.size(); ++i) {
                writeData[i] = static_cast<uint8_t>(i & 0xFF);
            }

            auto start = std::chrono::high_resolution_clock::now();
            auto result = device.scsiWrite10(0, sectors, writeData.data());
            auto end = std::chrono::high_resolution_clock::now();

            printScsiResult("WRITE " + std::to_string(sectors) + " sectors", result);

            if (result.success) {
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                if (duration > 0) {
                    double throughputKBps = (writeData.size() * 1000.0) / (duration * 1024.0);
                    double throughputMBps = throughputKBps / 1024.0;
                    std::cout << "  " << std::fixed << std::setprecision(2) 
                              << writeData.size() << " bytes in " << duration << " ms\n";
                    std::cout << "  Throughput: " << throughputKBps << " KB/s (" 
                              << throughputMBps << " MB/s)\n";
                }
            }
        }
        else {
            std::cout << "  Unknown command: " << cmd << "\n";
            std::cout << "  Type 'help' for available commands.\n";
        }
    }

    device.disconnect();
    return 0;
}
