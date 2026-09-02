/**
 * @file rs485_lib.cpp
 * @brief Implementation file for Rs485Testing class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "rs485_lib.h"

#include <algorithm>
#include <chrono>
#include <fcntl.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (Rs485Testing::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

/// True if the tty name looks like a hardware serial port.
bool isHardwareTty(const std::string &name) {
  return hasPrefix(name, "ttyS") || hasPrefix(name, "ttyAMA") ||
         hasPrefix(name, "ttyUSB") || hasPrefix(name, "ttyACM") ||
         hasPrefix(name, "ttyTHS");
}

} // anonymous namespace

Rs485Testing::Rs485Testing()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

Rs485Testing::~Rs485Testing() {}

Rs485Testing::Rs485Testing(std::shared_ptr<SysfsProvider> sysfs,
                           std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

int Rs485Testing::openFirstSerialPort(std::string &path) const {
  if (!sysfs_)
    return -1;
  for (const auto &e : sysfs_->listDir("/sys/class/tty")) {
    if (!isHardwareTty(e))
      continue;
    const auto entries = sysfs_->listDir("/sys/class/tty/" + e);
    if (std::find(entries.begin(), entries.end(), "device") == entries.end())
      continue;
    const std::string devPath = "/dev/" + e;
    const int fd = ::open(devPath.c_str(), O_RDWR | O_NDELAY | O_NOCTTY);
    if (fd >= 0) {
      path = devPath;
      return fd;
    }
  }
  return -1;
}

TestOutput Rs485Testing::testSerialDeviceEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  long long count = 0;
  for (const auto &e : sysfs_->listDir("/sys/class/tty")) {
    if (!isHardwareTty(e))
      continue;
    const auto entries = sysfs_->listDir("/sys/class/tty/" + e);
    if (std::find(entries.begin(), entries.end(), "device") != entries.end())
      ++count;
  }
  if (count == 0)
    return makeSkipped("no hardware serial ports found");
  TestOutput out = makePassed("serial ports enumerated");
  addMetric(out, "port_count", count);
  return out;
}

TestOutput Rs485Testing::testRs485Query() {
  std::string path;
  const int fd = openFirstSerialPort(path);
  if (fd < 0)
    return makeSkipped("no openable serial port for RS-485 query");
  struct serial_rs485 rs485Conf {};
  const int rc = ::ioctl(fd, TIOCGRS485, &rs485Conf);
  ::close(fd);
  if (rc != 0)
    return makeSkipped("driver does not support RS-485 ioctl (" + path + ")");
  TestOutput out = makePassed("RS-485 ioctl supported");
  addMetric(out, "rs485_enabled", (rs485Conf.flags & SER_RS485_ENABLED) != 0);
  addDetail(out, "port", path);
  return out;
}

TestOutput Rs485Testing::testRtsOnSendFlag() {
  std::string path;
  const int fd = openFirstSerialPort(path);
  if (fd < 0)
    return makeSkipped("no openable serial port for RS-485 query");
  struct serial_rs485 rs485Conf {};
  const int rc = ::ioctl(fd, TIOCGRS485, &rs485Conf);
  ::close(fd);
  if (rc != 0)
    return makeSkipped("driver does not support RS-485 ioctl (" + path + ")");
  TestOutput out = makePassed("RTS-on-send flag read");
  addMetric(out, "rts_on_send", (rs485Conf.flags & SER_RS485_RTS_ON_SEND) != 0);
  return out;
}

TestOutput Rs485Testing::testRxDuringTxFlag() {
  std::string path;
  const int fd = openFirstSerialPort(path);
  if (fd < 0)
    return makeSkipped("no openable serial port for RS-485 query");
  struct serial_rs485 rs485Conf {};
  const int rc = ::ioctl(fd, TIOCGRS485, &rs485Conf);
  ::close(fd);
  if (rc != 0)
    return makeSkipped("driver does not support RS-485 ioctl (" + path + ")");
  TestOutput out = makePassed("RX-during-TX flag read");
  addMetric(out, "rx_during_tx",
            (rs485Conf.flags & SER_RS485_RX_DURING_TX) != 0);
  return out;
}

TestOutput Rs485Testing::testDriverBinding() {
  if (!sysfs_ || !cmd_)
    return makeSkipped("no providers");
  for (const auto &e : sysfs_->listDir("/sys/class/tty")) {
    if (!isHardwareTty(e))
      continue;
    const auto entries = sysfs_->listDir("/sys/class/tty/" + e);
    if (std::find(entries.begin(), entries.end(), "device") == entries.end())
      continue;
    const std::string link = runCommand(
        cmd_, "readlink -f /sys/class/tty/" + e + "/device/driver 2>/dev/null");
    if (link.empty())
      continue;
    const std::string driver = link.substr(link.find_last_of('/') + 1);
    TestOutput out = makePassed("driver binding resolved");
    addDetail(out, "port", e);
    addDetail(out, "driver", driver);
    return out;
  }
  return makeSkipped("no hardware serial port with a bound driver");
}

std::vector<std::string> Rs485Testing::getAvailableTests() {
  return {"testSerialDeviceEnumeration", "testRs485Query", "testRtsOnSendFlag",
          "testRxDuringTxFlag", "testDriverBinding"};
}

std::vector<std::string> Rs485Testing::getPeriodicSafeTests() {
  return {"testSerialDeviceEnumeration", "testRs485Query", "testRtsOnSendFlag",
          "testRxDuringTxFlag", "testDriverBinding"};
}

std::vector<std::string> Rs485Testing::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> Rs485Testing::runAllTests() {
  std::vector<TestResult> results;
  for (const auto &name : getAvailableTests())
    results.push_back(runSingleTest(name));
  return results;
}

TestResult Rs485Testing::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testSerialDeviceEnumeration",
       &Rs485Testing::testSerialDeviceEnumeration},
      {"testRs485Query", &Rs485Testing::testRs485Query},
      {"testRtsOnSendFlag", &Rs485Testing::testRtsOnSendFlag},
      {"testRxDuringTxFlag", &Rs485Testing::testRxDuringTxFlag},
      {"testDriverBinding", &Rs485Testing::testDriverBinding},
  };
  for (const auto &e : table) {
    if (testName == e.name) {
      out = (this->*e.fn)();
      break;
    }
  }

  TestResult r;
  r.name = testName;
  r.test_id = testName;
  r.output = out;
  r.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::high_resolution_clock::now() - start);
  return r;
}
