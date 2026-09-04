/**
 * @file bluetooth_lib.cpp
 * @brief Implementation file for BluetoothTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include <iterator>
#include "bluetooth_lib.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (BluetoothTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

/// Lowercase copy of s.
std::string lowered(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

} // anonymous namespace

BluetoothTesting::BluetoothTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

BluetoothTesting::~BluetoothTesting() {}

BluetoothTesting::BluetoothTesting(std::shared_ptr<SysfsProvider> sysfs,
                                   std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

TestOutput BluetoothTesting::testAdapterEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  long long count = 0;
  for (const auto &e : sysfs_->listDir("/sys/class/bluetooth")) {
    if (hasPrefix(e, "hci"))
      ++count;
  }
  if (count == 0)
    return makeSkipped("no Bluetooth adapters present");
  TestOutput out = makePassed("Bluetooth adapters enumerated");
  addMetric(out, "adapter_count", count);
  return out;
}

TestOutput BluetoothTesting::testAdapterAddress() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  std::string adapter;
  for (const auto &e : sysfs_->listDir("/sys/class/bluetooth")) {
    if (hasPrefix(e, "hci")) {
      adapter = e;
      break;
    }
  }
  if (adapter.empty())
    return makeSkipped("no Bluetooth adapters present");
  const std::string addr =
      readSysfs(sysfs_, "/sys/class/bluetooth/" + adapter + "/address");
  if (addr.empty())
    return makeFailed("adapter address attribute missing");
  TestOutput out = makePassed("adapter address read");
  addDetail(out, "adapter", adapter);
  addDetail(out, "address", addr);
  return out;
}

TestOutput BluetoothTesting::testRfkillState() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto rfkills = sysfs_->listDir("/sys/class/rfkill");
  for (const auto &rk : rfkills) {
    const std::string rkDir = "/sys/class/rfkill/" + rk;
    const std::string type = lowered(readSysfs(sysfs_, rkDir + "/type"));
    if (type.find("bluetooth") == std::string::npos)
      continue;
    const long long soft = readSysfsInt(sysfs_, rkDir + "/soft");
    const long long hard = readSysfsInt(sysfs_, rkDir + "/hard");
    TestOutput out = makePassed("rfkill bluetooth state read");
    addMetric(out, "soft_blocked", soft == 1);
    addMetric(out, "hard_blocked", hard == 1);
    return out;
  }
  return makeSkipped("no bluetooth rfkill switch present");
}

TestOutput BluetoothTesting::testPoweredState() {
  if (!cmd_)
    return makeSkipped("no command provider");
  if (!sysfs_ || sysfs_->listDir("/sys/class/bluetooth").empty())
    return makeSkipped("no Bluetooth adapters present");
  const std::string out = runCommand(cmd_, "hciconfig 2>/dev/null");
  if (out.empty())
    return makeSkipped("hciconfig unavailable");
  const bool up = out.find("UP") != std::string::npos ||
                  out.find("RUNNING") != std::string::npos;
  TestOutput o = makePassed(up ? "adapter powered and running"
                               : "adapter present but down");
  addMetric(o, "powered", up);
  return o;
}

TestOutput BluetoothTesting::testBluezDaemon() {
  if (!cmd_)
    return makeSkipped("no command provider");
  const std::string out =
      runCommand(cmd_, "systemctl is-active bluetooth 2>/dev/null");
  if (out.empty())
    return makeSkipped("systemctl unavailable");
  const std::string state = lowered(out);
  if (state.find("active") != std::string::npos &&
      state.find("inactive") == std::string::npos) {
    TestOutput out2 = makePassed("BlueZ daemon active");
    return out2;
  }
  return makeSkipped("BlueZ daemon not active (state: " + state + ")");
}

std::vector<std::string> BluetoothTesting::getAvailableTests() {
  return {"testAdapterEnumeration", "testAdapterAddress", "testRfkillState",
          "testPoweredState", "testBluezDaemon"};
}

std::vector<std::string> BluetoothTesting::getPeriodicSafeTests() {
  return getAvailableTests();
}

std::vector<std::string> BluetoothTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> BluetoothTesting::runAllTests() {
  const std::vector<std::string> names = getAvailableTests();
  std::vector<TestResult> results;
  results.reserve(names.size());
  std::transform(names.begin(), names.end(), std::back_inserter(results),
                 [this](const std::string &name) { return runSingleTest(name); });
  return results;
}

TestResult BluetoothTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testAdapterEnumeration", &BluetoothTesting::testAdapterEnumeration},
      {"testAdapterAddress", &BluetoothTesting::testAdapterAddress},
      {"testRfkillState", &BluetoothTesting::testRfkillState},
      {"testPoweredState", &BluetoothTesting::testPoweredState},
      {"testBluezDaemon", &BluetoothTesting::testBluezDaemon},
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
