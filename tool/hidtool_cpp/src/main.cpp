/**
 * M487 HID Tool — Test CLI
 *
 * Verifies basic connectivity and I2C operations with the
 * M487 USB Composite Device (VID=0x04F3, PID=0x0732).
 *
 * Usage:
 *   M487HidTool                  — enumerate + connect + dummy read
 *   M487HidTool enum             — list matching HID interfaces
 *   M487HidTool read  <addr> <reg> [len]
 *   M487HidTool write <addr> <reg> <byte>...
 *   M487HidTool scan             — probe I2C addresses 0x03..0x77
 *   M487HidTool feature          — send/receive a Feature Report test
 */

#include "m487_hid_device.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void printHex(const uint8_t* data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (i > 0 && i % 16 == 0) std::cout << "\n       ";
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(data[i]) << ' ';
    }
    std::cout << std::dec << '\n';
}

static uint8_t parseHexByte(const char* s)
{
    return static_cast<uint8_t>(std::strtoul(s, nullptr, 16));
}

// ---------------------------------------------------------------------------
// Sub-commands
// ---------------------------------------------------------------------------

static int cmdEnumerate(M487HidDevice& dev)
{
    int n = dev.enumerate();
    if (n == 0) {
        std::cout << "[INFO] No M487 devices found (VID=0x04F3, PID=0x0732).\n";
        return 1;
    }

    std::cout << "[INFO] Found " << n << " device(s):\n\n";
    int idx = 0;
    for (const auto& d : dev.getDevices()) {
        std::cout << "  #" << idx++ << ": VID=0x"
                  << std::hex << std::setw(4) << std::setfill('0') << d.vendorId
                  << " PID=0x"
                  << std::setw(4) << std::setfill('0') << d.productId
                  << std::dec;
        if (!d.product.empty())
            std::cout << " \"" << d.product << "\"";
        std::cout << '\n';
        if (!d.manufacturer.empty())
            std::cout << "       Manufacturer: " << d.manufacturer << '\n';
        if (!d.serialNumber.empty())
            std::cout << "       Serial: " << d.serialNumber << '\n';
        std::cout << "       Path: " << d.path.substr(0, 70) << "...\n";
    }
    return 0;
}

static int cmdScan(M487HidDevice& dev)
{
    if (!dev.openFirst()) {
        std::cerr << "[ERROR] " << dev.lastError() << '\n';
        return 2;
    }
    std::cout << "[INFO] Connected. Scanning I2C bus...\n\n";
    std::cout << "     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n";

    for (int row = 0; row < 8; ++row) {
        std::cout << std::hex << row << "0: ";
        for (int col = 0; col < 16; ++col) {
            uint8_t addr = static_cast<uint8_t>(row * 16 + col);
            if (addr < 0x03 || addr > 0x77) {
                std::cout << "   ";
                continue;
            }
            uint8_t dummy = 0;
            if (dev.i2cRead(addr, 0x00, &dummy, 1))
                std::cout << std::setw(2) << std::setfill('0') << (int)addr << ' ';
            else
                std::cout << "-- ";
        }
        std::cout << '\n';
    }
    std::cout << std::dec;
    return 0;
}

static int cmdRead(M487HidDevice& dev, int argc, char* argv[])
{
    // read <addr> <reg> [len]
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " read <i2c_addr> <reg> [len]\n";
        return 1;
    }

    uint8_t addr = parseHexByte(argv[2]);
    uint8_t reg  = parseHexByte(argv[3]);
    uint8_t len  = (argc >= 5) ? static_cast<uint8_t>(std::atoi(argv[4])) : 1;
    if (len == 0) len = 1;

    if (!dev.openFirst()) {
        std::cerr << "[ERROR] " << dev.lastError() << '\n';
        return 2;
    }

    std::cout << "[INFO] I2C read: addr=0x" << std::hex << (int)addr
              << ", reg=0x" << (int)reg << ", len=" << std::dec << (int)len << '\n';

    uint8_t buf[64] = {};
    if (!dev.i2cRead(addr, reg, buf, len)) {
        std::cerr << "[ERROR] " << dev.lastError() << '\n';
        return 3;
    }

    std::cout << "[DATA] ";
    printHex(buf, len);
    return 0;
}

static int cmdWrite(M487HidDevice& dev, int argc, char* argv[])
{
    // write <addr> <reg> <byte>...
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " write <i2c_addr> <reg> <byte>...\n";
        return 1;
    }

    uint8_t addr = parseHexByte(argv[2]);
    uint8_t reg  = parseHexByte(argv[3]);

    uint8_t data[59] = {};
    int dataLen = argc - 4;
    if (dataLen > 59) dataLen = 59;
    for (int i = 0; i < dataLen; ++i)
        data[i] = parseHexByte(argv[4 + i]);

    if (!dev.openFirst()) {
        std::cerr << "[ERROR] " << dev.lastError() << '\n';
        return 2;
    }

    std::cout << "[INFO] I2C write: addr=0x" << std::hex << (int)addr
              << ", reg=0x" << (int)reg << ", len=" << std::dec << dataLen << '\n';

    if (!dev.i2cWrite(addr, reg, data, static_cast<uint8_t>(dataLen))) {
        std::cerr << "[ERROR] " << dev.lastError() << '\n';
        return 3;
    }

    std::cout << "[OK]   " << dataLen << " bytes written\n";
    return 0;
}

static int cmdFeatureTest(M487HidDevice& dev)
{
    if (!dev.openFirst()) {
        std::cerr << "[ERROR] " << dev.lastError() << '\n';
        return 2;
    }

    std::cout << "[INFO] Sending Feature Report (Report ID=0x05, Control Command)...\n";

    FeatureReport rpt{};
    rpt.reportId = REPORT_ID_CONTROL;
    rpt.payload[0] = 0x01;  // Dummy control command

    if (!dev.sendFeatureReport(reinterpret_cast<const uint8_t*>(&rpt), sizeof(rpt))) {
        std::cerr << "[WARN] SetFeature: " << dev.lastError() << '\n';
    } else {
        std::cout << "[OK]   Feature Report sent.\n";
    }

    std::cout << "[INFO] Reading Feature Report (Report ID=0x06, Status)...\n";
    FeatureReport resp{};
    resp.reportId = REPORT_ID_STATUS;

    if (!dev.recvFeatureReport(reinterpret_cast<uint8_t*>(&resp), sizeof(resp))) {
        std::cerr << "[WARN] GetFeature: " << dev.lastError() << '\n';
    } else {
        std::cout << "[OK]   Feature Report received:\n       ";
        printHex(reinterpret_cast<const uint8_t*>(&resp), 16);
    }

    return 0;
}

// ---------------------------------------------------------------------------
// Default: enumerate + quick connectivity test
// ---------------------------------------------------------------------------

static int cmdDefault(M487HidDevice& dev)
{
    std::cout << "=== M487 HID Tool (VID=0x04F3, PID=0x0732) ===\n\n";

    int n = dev.enumerate();
    if (n == 0) {
        std::cout << "[INFO] No devices found. Is the M487 board connected?\n";
        return 1;
    }

    std::cout << "[INFO] Found " << n << " device(s).\n";
    const auto& d = dev.getDevices()[0];
    std::cout << "[INFO] Using: " << (d.product.empty() ? "(unnamed)" : d.product) << '\n';

    if (!dev.open(0)) {
        std::cerr << "[ERROR] " << dev.lastError() << '\n';
        return 2;
    }
    std::cout << "[OK]   Device handle opened.\n";

    // Quick dummy read to verify communication
    std::cout << "[INFO] Sending test read (addr=0x00, reg=0x00, len=1)...\n";
    uint8_t dummy = 0;
    if (dev.i2cRead(0x00, 0x00, &dummy, 1)) {
        std::cout << "[OK]   Got response: 0x"
                  << std::hex << std::setw(2) << std::setfill('0')
                  << (int)dummy << std::dec << '\n';
    } else {
        std::cout << "[WARN] " << dev.lastError() << '\n';
        std::cout << "       (This is expected if no I2C slave at address 0x00)\n";
    }

    std::cout << "\n[DONE] Basic connectivity test complete.\n";
    return 0;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    M487HidDevice dev;

    if (argc < 2)
        return cmdDefault(dev);

    std::string cmd = argv[1];
    if (cmd == "enum" || cmd == "list")
        return cmdEnumerate(dev);
    if (cmd == "scan")
        return cmdScan(dev);
    if (cmd == "read")
        return cmdRead(dev, argc, argv);
    if (cmd == "write")
        return cmdWrite(dev, argc, argv);
    if (cmd == "feature")
        return cmdFeatureTest(dev);

    std::cerr << "Unknown command: " << cmd << "\n\n"
              << "Usage: " << argv[0] << " [enum|scan|read|write|feature]\n";
    return 1;
}
