/**
 * @file can_bus_lib.cpp
 * @brief Implementation file for CanBusTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include <iterator>
#include "can_bus_lib.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (CanBusTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

} // anonymous namespace

CanBusTesting::CanBusTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

CanBusTesting::~CanBusTesting() {}

CanBusTesting::CanBusTesting(std::shared_ptr<SysfsProvider> sysfs,
                             std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string CanBusTesting::firstCanInterface() const {
  if (!sysfs_)
    return "";
  for (const auto &e : sysfs_->listDir("/sys/class/net")) {
    if (hasPrefix(e, "can"))
      return e;
  }
  return "";
}

TestOutput CanBusTesting::testInterfaceEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  std::vector<std::string> ifaces;
  for (const auto &e : sysfs_->listDir("/sys/class/net")) {
    if (hasPrefix(e, "can"))
      ifaces.push_back(e);
  }
  if (ifaces.empty())
    return makeSkipped("no CAN network interfaces present");
  TestOutput out = makePassed("CAN interfaces enumerated");
  addMetric(out, "iface_count", static_cast<int64_t>(ifaces.size()));
  addDetail(out, "first_iface", ifaces.front());
  return out;
}

TestOutput CanBusTesting::testOperState() {
  const std::string iface = firstCanInterface();
  if (iface.empty())
    return makeSkipped("no CAN interfaces present");
  const std::string state =
      readSysfs(sysfs_, "/sys/class/net/" + iface + "/operstate");
  if (state.empty())
    return makeFailed("operstate attribute missing");
  TestOutput out = makePassed("interface operational state read");
  addDetail(out, "operstate", state);
  return out;
}

TestOutput CanBusTesting::testStatistics() {
  const std::string iface = firstCanInterface();
  if (iface.empty())
    return makeSkipped("no CAN interfaces present");
  const std::string statsDir = "/sys/class/net/" + iface + "/statistics";
  const long long rx = readSysfsInt(sysfs_, statsDir + "/rx_packets");
  const long long tx = readSysfsInt(sysfs_, statsDir + "/tx_packets");
  if (rx < 0 && tx < 0)
    return makeSkipped("statistics unavailable for " + iface);
  const long long rxErr = readSysfsInt(sysfs_, statsDir + "/rx_errors");
  const long long txErr = readSysfsInt(sysfs_, statsDir + "/tx_errors");
  TestOutput out = makePassed("interface statistics read");
  addMetric(out, "rx_packets", rx < 0 ? 0LL : rx);
  addMetric(out, "tx_packets", tx < 0 ? 0LL : tx);
  addMetric(out, "rx_errors", rxErr < 0 ? 0LL : rxErr);
  addMetric(out, "tx_errors", txErr < 0 ? 0LL : txErr);
  return out;
}

TestOutput CanBusTesting::testBitrateRead() {
  if (!cmd_)
    return makeSkipped("no command provider");
  const std::string iface = firstCanInterface();
  if (iface.empty())
    return makeSkipped("no CAN interfaces present");
  if (!isSafeName(iface))
    return makeFailed("suspicious CAN interface name, refusing to run ip: " +
                      iface);
  const std::string out =
      runCommand(cmd_, "ip -details link show " + iface + " 2>/dev/null");
  if (out.empty())
    return makeSkipped("ip utility unavailable or link query failed");
  const std::size_t pos = out.find("bitrate");
  if (pos == std::string::npos)
    return makeSkipped("no bitrate configured on " + iface);
  long long bitrate = -1;
  std::istringstream ss(out.substr(pos));
  std::string tok;
  while (ss >> tok) {
    try {
      bitrate = std::stoll(tok);
      break;
    } catch (...) {
      continue;
    }
  }
  if (bitrate <= 0)
    return makeSkipped("could not parse bitrate for " + iface);
  TestOutput o = makePassed("bitrate read");
  addMetric(o, "bitrate_bps", static_cast<double>(bitrate));
  return o;
}

TestOutput CanBusTesting::testCanState() {
  const std::string iface = firstCanInterface();
  if (iface.empty())
    return makeSkipped("no CAN interfaces present");
  const std::string state =
      readSysfs(sysfs_, "/sys/class/net/" + iface + "/state");
  if (state.empty())
    return makeSkipped("CAN state attribute unavailable for " + iface);
  TestOutput out = makePassed("CAN bus state read");
  addDetail(out, "can_state", state);
  return out;
}

std::vector<std::string> CanBusTesting::getAvailableTests() {
  return {"testInterfaceEnumeration", "testOperState", "testStatistics",
          "testBitrateRead", "testCanState"};
}

std::vector<std::string> CanBusTesting::getPeriodicSafeTests() {
  return getAvailableTests();
}

std::vector<std::string> CanBusTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> CanBusTesting::runAllTests() {
  const std::vector<std::string> names = getAvailableTests();
  std::vector<TestResult> results;
  results.reserve(names.size());
  std::transform(names.begin(), names.end(), std::back_inserter(results),
                 [this](const std::string &name) { return runSingleTest(name); });
  return results;
}

TestResult CanBusTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testInterfaceEnumeration", &CanBusTesting::testInterfaceEnumeration},
      {"testOperState", &CanBusTesting::testOperState},
      {"testStatistics", &CanBusTesting::testStatistics},
      {"testBitrateRead", &CanBusTesting::testBitrateRead},
      {"testCanState", &CanBusTesting::testCanState},
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
