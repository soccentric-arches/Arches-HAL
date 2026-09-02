/**
 * @file i2c_lib.cpp
 * @brief Implementation file for I2cTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "i2c_lib.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sstream>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (I2cTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

/// Extract integer bus number from a device name like "i2c-3".
int busNumber(const std::string &devName) {
  try {
    return std::stoi(devName.substr(4));
  } catch (...) {
    return -1;
  }
}

/// Count hex tokens in i2cdetect output (detected device addresses).
int countDetectedAddresses(const std::string &output) {
  std::istringstream ss(output);
  int count = 0;
  std::string tok;
  while (ss >> tok) {
    if (tok.size() == 2 && std::isxdigit(static_cast<unsigned char>(tok[0])) &&
        std::isxdigit(static_cast<unsigned char>(tok[1]))) {
      ++count;
    }
  }
  return count;
}

} // anonymous namespace

I2cTesting::I2cTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

I2cTesting::~I2cTesting() {}

I2cTesting::I2cTesting(std::shared_ptr<SysfsProvider> sysfs,
                       std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

TestOutput I2cTesting::testBusEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  std::vector<std::string> buses;
  for (const auto &e : sysfs_->listDir("/dev")) {
    if (hasPrefix(e, "i2c-"))
      buses.push_back(e);
  }
  if (buses.empty())
    return makeSkipped("no /dev/i2c-* adapter nodes found");
  TestOutput out = makePassed("I2C adapters present");
  addMetric(out, "bus_count", static_cast<int64_t>(buses.size()));
  addDetail(out, "first_bus", buses.front());
  return out;
}

TestOutput I2cTesting::testI2cDetectScan() {
  if (!sysfs_ || !cmd_)
    return makeSkipped("no providers");
  std::string firstBus;
  for (const auto &e : sysfs_->listDir("/dev")) {
    if (hasPrefix(e, "i2c-")) {
      firstBus = e;
      break;
    }
  }
  if (firstBus.empty())
    return makeSkipped("no I2C adapter nodes");
  const int bus = busNumber(firstBus);
  if (bus < 0)
    return makeSkipped("cannot parse bus number from " + firstBus);
  const std::string out =
      runCommand(cmd_, "i2cdetect -y " + std::to_string(bus));
  if (out.empty())
    return makeSkipped("i2cdetect unavailable or scan produced no output");
  TestOutput o = makePassed("bus scan completed");
  addMetric(o, "detected_addresses",
            static_cast<int64_t>(countDetectedAddresses(out)));
  return o;
}

TestOutput I2cTesting::testSysfsDevices() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/bus/i2c/devices");
  if (entries.empty())
    return makeSkipped("no devices on the i2c bus");
  TestOutput out = makePassed("i2c bus devices enumerated");
  addMetric(out, "device_count", static_cast<int64_t>(entries.size()));
  return out;
}

TestOutput I2cTesting::testAdapterNameRead() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  std::string firstAdapter;
  for (const auto &e : sysfs_->listDir("/sys/bus/i2c/devices")) {
    if (hasPrefix(e, "i2c-")) {
      firstAdapter = e;
      break;
    }
  }
  if (firstAdapter.empty())
    return makeSkipped("no i2c adapters in sysfs");
  const std::string name =
      readSysfs(sysfs_, "/sys/bus/i2c/devices/" + firstAdapter + "/name");
  if (name.empty())
    return makeFailed("adapter name file missing or empty");
  TestOutput out = makePassed("adapter name read");
  addDetail(out, "adapter_name", name);
  return out;
}

TestOutput I2cTesting::testDriverBinding() {
  if (!sysfs_ || !cmd_)
    return makeSkipped("no providers");
  const auto entries = sysfs_->listDir("/sys/bus/i2c/devices");
  std::vector<std::string> clients;
  for (const auto &e : entries) {
    if (!hasPrefix(e, "i2c-"))
      clients.push_back(e);
  }
  if (clients.empty())
    return makeSkipped("no i2c client devices to check");
  const std::string boundStr = runCommand(
      cmd_, "ls -d /sys/bus/i2c/devices/*/driver 2>/dev/null | wc -l");
  long long boundCount = -1;
  try {
    boundCount = std::stoll(boundStr);
  } catch (...) {
    boundCount = -1;
  }
  if (boundCount < 0)
    return makeSkipped("cannot query driver bindings");
  TestOutput out = makePassed("driver binding query complete");
  addMetric(out, "clients", static_cast<int64_t>(clients.size()));
  addMetric(out, "bound_clients", boundCount);
  return out;
}

std::vector<std::string> I2cTesting::getAvailableTests() {
  return {"testBusEnumeration", "testI2cDetectScan", "testSysfsDevices",
          "testAdapterNameRead", "testDriverBinding"};
}

std::vector<std::string> I2cTesting::getPeriodicSafeTests() {
  return {"testBusEnumeration", "testSysfsDevices", "testAdapterNameRead",
          "testDriverBinding"};
}

std::vector<std::string> I2cTesting::getOneShotOnlyTests() {
  return {"testI2cDetectScan"};
}

std::vector<TestResult> I2cTesting::runAllTests() {
  std::vector<TestResult> results;
  for (const auto &name : getAvailableTests())
    results.push_back(runSingleTest(name));
  return results;
}

TestResult I2cTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testBusEnumeration", &I2cTesting::testBusEnumeration},
      {"testI2cDetectScan", &I2cTesting::testI2cDetectScan},
      {"testSysfsDevices", &I2cTesting::testSysfsDevices},
      {"testAdapterNameRead", &I2cTesting::testAdapterNameRead},
      {"testDriverBinding", &I2cTesting::testDriverBinding},
  };
  bool known = false;
  for (const auto &e : table) {
    if (testName == e.name) {
      out = (this->*e.fn)();
      known = true;
      break;
    }
  }
  (void)known;

  TestResult r;
  r.name = testName;
  r.test_id = testName;
  r.output = out;
  r.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::high_resolution_clock::now() - start);
  return r;
}
