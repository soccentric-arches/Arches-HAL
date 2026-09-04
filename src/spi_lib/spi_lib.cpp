/**
 * @file spi_lib.cpp
 * @brief Implementation file for SpiTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include <algorithm>
#include <iterator>
#include "spi_lib.h"

#include <chrono>
#include <sstream>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (SpiTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

/// First entry in a directory whose name starts with prefix ("" if none).
std::string firstWithPrefix(const std::shared_ptr<SysfsProvider> &sysfs,
                            const std::string &dir, const std::string &prefix) {
  if (!sysfs)
    return "";
  for (const auto &e : sysfs->listDir(dir)) {
    if (hasPrefix(e, prefix))
      return e;
  }
  return "";
}

} // anonymous namespace

SpiTesting::SpiTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

SpiTesting::~SpiTesting() {}

SpiTesting::SpiTesting(std::shared_ptr<SysfsProvider> sysfs,
                       std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

TestOutput SpiTesting::testSpidevEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  std::vector<std::string> spidevs;
  for (const auto &e : sysfs_->listDir("/dev")) {
    if (hasPrefix(e, "spidev"))
      spidevs.push_back(e);
  }
  if (spidevs.empty())
    return makeSkipped("no /dev/spidev* nodes found");
  TestOutput out = makePassed("spidev nodes present");
  addMetric(out, "spidev_count", static_cast<int64_t>(spidevs.size()));
  addDetail(out, "first_device", spidevs.front());
  return out;
}

TestOutput SpiTesting::testSysfsDevices() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/bus/spi/devices");
  if (entries.empty())
    return makeSkipped("no devices on the spi bus");
  TestOutput out = makePassed("spi bus devices enumerated");
  addMetric(out, "device_count", static_cast<int64_t>(entries.size()));
  return out;
}

TestOutput SpiTesting::testModaliasRead() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const std::string dev =
      firstWithPrefix(sysfs_, "/sys/bus/spi/devices", "spi");
  if (dev.empty())
    return makeSkipped("no spi devices in sysfs");
  const std::string modalias =
      readSysfs(sysfs_, "/sys/bus/spi/devices/" + dev + "/modalias");
  if (modalias.empty())
    return makeFailed("modalias file missing or empty");
  TestOutput out = makePassed("modalias read");
  addDetail(out, "device", dev);
  addDetail(out, "modalias", modalias);
  return out;
}

TestOutput SpiTesting::testLoopbackTransfer() {
  if (!sysfs_ || !cmd_)
    return makeSkipped("no providers");
  const std::string dev = firstWithPrefix(sysfs_, "/dev", "spidev");
  if (dev.empty())
    return makeSkipped("no spidev device node for loopback");
  if (!isSafeName(dev))
    return makeSkipped("suspicious spidev name, refusing to run spidev_test");
  const std::string out =
      runCommand(cmd_, "spidev_test -D /dev/" + dev + " -v -s 1000000 2>&1");
  if (out.empty())
    return makeSkipped("spidev_test unavailable or produced no output");
  if (out.find("can't") != std::string::npos ||
      out.find("No such") != std::string::npos ||
      out.find("Permission") != std::string::npos) {
    return makeSkipped("spidev_test could not access device: " + dev);
  }
  if (out.find("TX") == std::string::npos) {
    return makeFailed("unexpected spidev_test output");
  }
  const bool loopback = out.find("RX") != std::string::npos;
  TestOutput o =
      makePassed(loopback ? "transfer executed; RX seen (loopback or echo)"
                          : "transfer executed");
  addDetail(o, "device", dev);
  return o;
}

TestOutput SpiTesting::testDriverBinding() {
  if (!sysfs_ || !cmd_)
    return makeSkipped("no providers");
  const auto entries = sysfs_->listDir("/sys/bus/spi/devices");
  if (entries.empty())
    return makeSkipped("no spi devices to check");
  const std::string boundStr = runCommand(
      cmd_, "ls -d /sys/bus/spi/devices/*/driver 2>/dev/null | wc -l");
  long long boundCount = -1;
  try {
    boundCount = std::stoll(boundStr);
  } catch (...) {
    boundCount = -1;
  }
  if (boundCount < 0)
    return makeSkipped("cannot query driver bindings");
  TestOutput out = makePassed("driver binding query complete");
  addMetric(out, "devices", static_cast<int64_t>(entries.size()));
  addMetric(out, "bound_devices", boundCount);
  return out;
}

std::vector<std::string> SpiTesting::getAvailableTests() {
  return {"testSpidevEnumeration", "testSysfsDevices", "testModaliasRead",
          "testLoopbackTransfer", "testDriverBinding"};
}

std::vector<std::string> SpiTesting::getPeriodicSafeTests() {
  return {"testSpidevEnumeration", "testSysfsDevices", "testModaliasRead",
          "testDriverBinding"};
}

std::vector<std::string> SpiTesting::getOneShotOnlyTests() {
  return {"testLoopbackTransfer"};
}

std::vector<TestResult> SpiTesting::runAllTests() {
  const std::vector<std::string> names = getAvailableTests();
  std::vector<TestResult> results;
  results.reserve(names.size());
  std::transform(names.begin(), names.end(), std::back_inserter(results),
                 [this](const std::string &name) { return runSingleTest(name); });
  return results;
}

TestResult SpiTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testSpidevEnumeration", &SpiTesting::testSpidevEnumeration},
      {"testSysfsDevices", &SpiTesting::testSysfsDevices},
      {"testModaliasRead", &SpiTesting::testModaliasRead},
      {"testLoopbackTransfer", &SpiTesting::testLoopbackTransfer},
      {"testDriverBinding", &SpiTesting::testDriverBinding},
  };
  const auto entry = std::find_if(table.begin(), table.end(),
      [&testName](const TestEntry &e) { return testName == e.name; });
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
