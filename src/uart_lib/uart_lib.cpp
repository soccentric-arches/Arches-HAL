/**
 * @file uart_lib.cpp
 * @brief Implementation file for UartTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "uart_lib.h"
#include <iterator>

#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace {

struct TestEntry {
    const char* name;
    TestOutput (UartTesting::*fn)();
};

bool hasPrefix(const std::string& s, const std::string& prefix) { return s.rfind(prefix, 0) == 0; }

/// True if the tty name looks like a hardware serial port (not a virtual tty).
bool isHardwareTty(const std::string& name) {
    return hasPrefix(name, "ttyS") || hasPrefix(name, "ttyAMA") || hasPrefix(name, "ttyUSB") ||
           hasPrefix(name, "ttyACM") || hasPrefix(name, "ttyTHS");
}

} // anonymous namespace

UartTesting::UartTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()), cmd_(std::make_shared<RealCommandProvider>()) {
}

UartTesting::~UartTesting() {}

UartTesting::UartTesting(std::shared_ptr<SysfsProvider> sysfs, std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string UartTesting::firstSerialPort() const {
    if (!sysfs_)
        return "";
    for (const auto& e : sysfs_->listDir("/sys/class/tty")) {
        if (!isHardwareTty(e))
            continue;
        // Real ports have a "device" symlink in their sysfs directory.
        const auto entries = sysfs_->listDir("/sys/class/tty/" + e);
        if (std::find(entries.begin(), entries.end(), "device") != entries.end())
            return e;
    }
    return "";
}

TestOutput UartTesting::testSerialPortEnumeration() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    std::vector<std::string> ports;
    for (const auto& e : sysfs_->listDir("/sys/class/tty")) {
        if (!isHardwareTty(e))
            continue;
        const auto entries = sysfs_->listDir("/sys/class/tty/" + e);
        if (std::find(entries.begin(), entries.end(), "device") != entries.end())
            ports.push_back(e);
    }
    if (ports.empty())
        return makeSkipped("no hardware serial ports found");
    TestOutput out = makePassed("serial ports enumerated");
    addMetric(out, "port_count", static_cast<int64_t>(ports.size()));
    addDetail(out, "first_port", ports.front());
    return out;
}

TestOutput UartTesting::testConsoleDetection() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    const std::string cmdline = readSysfs(sysfs_, "/proc/cmdline");
    if (cmdline.empty())
        return makeSkipped("cannot read /proc/cmdline");
    std::istringstream ss(cmdline);
    std::vector<std::string> consoles;
    std::string tok;
    while (ss >> tok) {
        if (hasPrefix(tok, "console="))
            consoles.push_back(tok.substr(8));
    }
    if (consoles.empty())
        return makeSkipped("no console= configured on the kernel command line");
    TestOutput out = makePassed("console device(s) configured");
    addMetric(out, "console_count", static_cast<int64_t>(consoles.size()));
    addDetail(out, "first_console", consoles.front());
    return out;
}

TestOutput UartTesting::testSerialPortOpen() {
    const std::string port = firstSerialPort();
    if (port.empty())
        return makeSkipped("no hardware serial port to open");
    const std::string path = "/dev/" + port;
    const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOCTTY);
    if (fd < 0) {
        return makeFailed("open(" + path + ") failed");
    }
    ::close(fd);
    TestOutput out = makePassed("serial port opened successfully");
    addDetail(out, "port", path);
    return out;
}

TestOutput UartTesting::testBaudRateRead() {
    if (!cmd_)
        return makeSkipped("no command provider");
    const std::string port = firstSerialPort();
    if (port.empty())
        return makeSkipped("no hardware serial port to query");
    if (!isSafeName(port))
        return makeSkipped("suspicious serial port name, refusing to run stty");
    const std::string out = runCommand(cmd_, "stty -F /dev/" + port + " 2>/dev/null");
    if (out.empty())
        return makeSkipped("stty unavailable or failed for /dev/" + port);
    long long baud = -1;
    std::istringstream ss(out);
    std::string tok;
    if (ss >> tok) {
        try {
            baud = std::stoll(tok);
        } catch (...) {
            baud = -1;
        }
    }
    if (baud <= 0)
        return makeSkipped("could not parse baud rate from stty output");
    TestOutput o = makePassed("baud rate read");
    addMetric(o, "baud", baud);
    addDetail(o, "port", "/dev/" + port);
    return o;
}

TestOutput UartTesting::testDriverBinding() {
    if (!cmd_)
        return makeSkipped("no command provider");
    const std::string port = firstSerialPort();
    if (port.empty())
        return makeSkipped("no hardware serial port to check");
    if (!isSafeName(port))
        return makeSkipped("suspicious serial port name, refusing to run readlink");
    const std::string link =
        runCommand(cmd_, "readlink -f /sys/class/tty/" + port + "/device/driver 2>/dev/null");
    if (link.empty())
        return makeSkipped("no driver bound to " + port);
    const std::string driver = link.substr(link.find_last_of('/') + 1);
    TestOutput out = makePassed("driver binding resolved");
    addDetail(out, "port", port);
    addDetail(out, "driver", driver);
    return out;
}

std::vector<std::string> UartTesting::getAvailableTests() {
    return {"testSerialPortEnumeration", "testConsoleDetection", "testSerialPortOpen",
            "testBaudRateRead", "testDriverBinding"};
}

std::vector<std::string> UartTesting::getPeriodicSafeTests() {
    return {"testSerialPortEnumeration", "testConsoleDetection", "testBaudRateRead",
            "testDriverBinding"};
}

std::vector<std::string> UartTesting::getOneShotOnlyTests() { return {"testSerialPortOpen"}; }

std::vector<TestResult> UartTesting::runAllTests() {
    const std::vector<std::string> names = getAvailableTests();
    std::vector<TestResult> results;
    results.reserve(names.size());
    std::transform(names.begin(), names.end(), std::back_inserter(results),
                   [this](const std::string& name) { return runSingleTest(name); });
    return results;
}

TestResult UartTesting::runSingleTest(const std::string& testName) {
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput out;
    out.status = hal::Status::ERROR;
    out.severity = hal::Severity::ERROR;
    out.error_message = "Unknown test name: " + testName;

    const std::vector<TestEntry> table = {
        {"testSerialPortEnumeration", &UartTesting::testSerialPortEnumeration},
        {"testConsoleDetection", &UartTesting::testConsoleDetection},
        {"testSerialPortOpen", &UartTesting::testSerialPortOpen},
        {"testBaudRateRead", &UartTesting::testBaudRateRead},
        {"testDriverBinding", &UartTesting::testDriverBinding},
    };
    const auto entry = std::find_if(table.begin(), table.end(),
                                    [&testName](const TestEntry& e) { return testName == e.name; });
    if (entry != table.end())
        out = (this->*entry->fn)();

    TestResult r;
    r.name = testName;
    r.test_id = testName;
    r.output = out;
    r.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::high_resolution_clock::now() - start);
    return r;
}
