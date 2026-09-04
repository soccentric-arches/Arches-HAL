/**
 * @file battery_lib.cpp
 * @brief Implementation file for BatteryTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "battery_lib.h"

#include <chrono>
#include <cmath>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (BatteryTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

const char *const kKnownStatuses[] = {"Charging", "Discharging", "Not charging",
                                      "Full", "Unknown"};

} // anonymous namespace

BatteryTesting::BatteryTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

BatteryTesting::~BatteryTesting() {}

BatteryTesting::BatteryTesting(std::shared_ptr<SysfsProvider> sysfs,
                               std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string BatteryTesting::firstBatterySupply() const {
  if (!sysfs_)
    return "";
  for (const auto &e : sysfs_->listDir("/sys/class/power_supply")) {
    if (!hasPrefix(e, "BAT"))
      continue;
    if (readSysfsInt(sysfs_, "/sys/class/power_supply/" + e + "/capacity") >= 0)
      return e;
    if (!readSysfs(sysfs_, "/sys/class/power_supply/" + e + "/status").empty())
      return e;
  }
  return "";
}

TestOutput BatteryTesting::testPowerSupplyEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/class/power_supply");
  if (entries.empty())
    return makeSkipped("no power supplies present");
  TestOutput out = makePassed("power supplies enumerated");
  addMetric(out, "supply_count", static_cast<int64_t>(entries.size()));
  return out;
}

TestOutput BatteryTesting::testBatteryPresence() {
  const std::string bat = firstBatterySupply();
  if (bat.empty())
    return makeSkipped("no battery power supply present");
  TestOutput out = makePassed("battery supply present");
  addDetail(out, "supply", bat);
  return out;
}

TestOutput BatteryTesting::testCapacityRead() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const std::string bat = firstBatterySupply();
  if (bat.empty())
    return makeSkipped("no battery power supply present");
  const long long capacity =
      readSysfsInt(sysfs_, "/sys/class/power_supply/" + bat + "/capacity");
  if (capacity < 0)
    return makeFailed("capacity attribute unreadable");
  if (capacity > 100)
    return makeFailed("capacity out of range: " + std::to_string(capacity));
  TestOutput out = makePassed("capacity within valid range");
  addMetric(out, "capacity_percent", static_cast<double>(capacity));
  return out;
}

TestOutput BatteryTesting::testVoltageCurrent() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const std::string bat = firstBatterySupply();
  if (bat.empty())
    return makeSkipped("no battery power supply present");
  const std::string base = "/sys/class/power_supply/" + bat;
  const long long uv = readSysfsInt(sysfs_, base + "/voltage_now");
  if (uv <= 0)
    return makeSkipped("voltage_now unavailable");
  TestOutput out = makePassed("voltage read");
  addMetric(out, "voltage_V", static_cast<double>(uv) / 1e6);
  const long long ua = readSysfsInt(sysfs_, base + "/current_now");
  if (ua > 0)
    addMetric(out, "current_A", static_cast<double>(ua) / 1e6);
  return out;
}

TestOutput BatteryTesting::testBatteryHealth() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const std::string bat = firstBatterySupply();
  if (bat.empty())
    return makeSkipped("no battery power supply present");
  const std::string base = "/sys/class/power_supply/" + bat;
  long long full = readSysfsInt(sysfs_, base + "/energy_full");
  long long design = readSysfsInt(sysfs_, base + "/energy_full_design");
  if (full <= 0 || design <= 0) {
    full = readSysfsInt(sysfs_, base + "/charge_full");
    design = readSysfsInt(sysfs_, base + "/charge_full_design");
  }
  if (full <= 0 || design <= 0)
    return makeSkipped("battery health attributes unavailable");
  const double health =
      100.0 * static_cast<double>(full) / static_cast<double>(design);
  TestOutput out = makePassed(health >= 50.0 ? "battery health acceptable"
                                             : "battery health degraded");
  if (health < 50.0) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::WARNING;
  }
  addMetric(out, "health_percent", health);
  return out;
}

TestOutput BatteryTesting::testStatusRead() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const std::string bat = firstBatterySupply();
  if (bat.empty())
    return makeSkipped("no battery power supply present");
  const std::string status =
      readSysfs(sysfs_, "/sys/class/power_supply/" + bat + "/status");
  bool known = false;
  for (const auto &s : kKnownStatuses) {
    if (status == s)
      known = true;
  }
  if (status.empty())
    return makeFailed("status attribute missing");
  if (!known)
    return makeSkipped("unknown status string: " + status);
  TestOutput out = makePassed("status read");
  addDetail(out, "status", status);
  return out;
}

std::vector<std::string> BatteryTesting::getAvailableTests() {
  return {"testPowerSupplyEnumeration", "testBatteryPresence",
          "testCapacityRead",           "testVoltageCurrent",
          "testBatteryHealth",          "testStatusRead"};
}

std::vector<std::string> BatteryTesting::getPeriodicSafeTests() {
  return getAvailableTests();
}

std::vector<std::string> BatteryTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> BatteryTesting::runAllTests() {
  std::vector<TestResult> results;
  for (const auto &name : getAvailableTests())
    results.push_back(runSingleTest(name));
  return results;
}

TestResult BatteryTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testPowerSupplyEnumeration",
       &BatteryTesting::testPowerSupplyEnumeration},
      {"testBatteryPresence", &BatteryTesting::testBatteryPresence},
      {"testCapacityRead", &BatteryTesting::testCapacityRead},
      {"testVoltageCurrent", &BatteryTesting::testVoltageCurrent},
      {"testBatteryHealth", &BatteryTesting::testBatteryHealth},
      {"testStatusRead", &BatteryTesting::testStatusRead},
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
