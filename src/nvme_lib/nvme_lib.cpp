/**
 * @file nvme_lib.cpp
 * @brief Implementation file for NvmeTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "nvme_lib.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (NvmeTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

} // anonymous namespace

NvmeTesting::NvmeTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

NvmeTesting::~NvmeTesting() {}

NvmeTesting::NvmeTesting(std::shared_ptr<SysfsProvider> sysfs,
                         std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

TestOutput NvmeTesting::testControllerEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/class/nvme");
  if (entries.empty())
    return makeSkipped("no NVMe controllers present");
  TestOutput out = makePassed("NVMe controllers enumerated");
  addMetric(out, "controller_count", static_cast<int64_t>(entries.size()));
  return out;
}

TestOutput NvmeTesting::testNamespaceEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  std::string firstCtrl;
  for (const auto &e : sysfs_->listDir("/sys/class/nvme")) {
    if (hasPrefix(e, "nvme")) {
      firstCtrl = e;
      break;
    }
  }
  if (firstCtrl.empty())
    return makeSkipped("no NVMe controllers present");
  const auto entries = sysfs_->listDir("/sys/class/nvme/" + firstCtrl);
  long long namespaces = 0;
  for (const auto &e : entries) {
    if (e.size() > firstCtrl.size() && e.rfind(firstCtrl, 0) == 0 &&
        e[firstCtrl.size()] == 'n')
      ++namespaces;
  }
  TestOutput out = makePassed(namespaces > 0 ? "namespaces enumerated"
                                             : "controller has no namespaces");
  addMetric(out, "namespace_count", namespaces);
  addDetail(out, "controller", firstCtrl);
  return out;
}

TestOutput NvmeTesting::testControllerState() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/class/nvme");
  if (entries.empty())
    return makeSkipped("no NVMe controllers present");
  const std::string state =
      readSysfs(sysfs_, "/sys/class/nvme/" + entries.front() + "/state");
  if (state.empty())
    return makeFailed("controller state attribute missing");
  if (state != "live")
    return makeFailed("controller state is '" + state + "' (expected live)");
  TestOutput out = makePassed("controller state is live");
  addDetail(out, "state", state);
  return out;
}

TestOutput NvmeTesting::testSmartHealth() {
  if (!cmd_)
    return makeSkipped("no command provider");
  if (!sysfs_ || sysfs_->listDir("/sys/class/nvme").empty())
    return makeSkipped("no NVMe controllers present");
  const std::string out = runCommand(cmd_, "nvme smart-log /dev/nvme0 2>&1");
  const bool hasCliMarker = out.find("nvme-cli") != std::string::npos;
  const bool hasSmartMarker = out.find("critical_warning") != std::string::npos;
  if (out.empty() || (!hasCliMarker && !hasSmartMarker))
    return makeSkipped("nvme-cli unavailable or log unreadable");
  // Parse "critical_warning : 0x0" — non-zero indicates a health problem.
  long long critical = 0;
  std::istringstream ss(out);
  std::string tok;
  while (ss >> tok) {
    if (tok == "critical_warning") {
      std::string sep, val;
      ss >> sep >> val;
      try {
        critical = std::stoll(val, nullptr, 0);
      } catch (...) {
        critical = -1;
      }
      break;
    }
  }
  if (critical < 0)
    return makeSkipped("critical_warning not found in SMART log");
  if (critical != 0)
    return makeFailed("SMART critical_warning is non-zero");
  TestOutput o = makePassed("SMART log clean");
  double tempC = -1.0;
  std::istringstream ss2(out);
  while (ss2 >> tok) {
    if (tok == "temperature") {
      std::string sep, val;
      ss2 >> sep >> val;
      try {
        tempC = std::stod(val) - 273.0;
      } catch (...) {
        tempC = -1.0;
      }
      break;
    }
  }
  if (tempC > 0)
    addMetric(o, "temperature_C", tempC);
  return o;
}

TestOutput NvmeTesting::testPartitionEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  if (sysfs_->listDir("/sys/class/nvme").empty())
    return makeSkipped("no NVMe controllers present");
  const std::string procParts = readSysfs(sysfs_, "/proc/partitions");
  if (procParts.empty())
    return makeSkipped("cannot read /proc/partitions");
  long long partitions = 0;
  std::istringstream ss(procParts);
  std::string line;
  while (std::getline(ss, line)) {
    if (line.find("nvme") != std::string::npos)
      ++partitions;
  }
  TestOutput out = makePassed("NVMe block devices counted");
  addMetric(out, "nvme_block_devices", partitions);
  return out;
}

std::vector<std::string> NvmeTesting::getAvailableTests() {
  return {"testControllerEnumeration", "testNamespaceEnumeration",
          "testControllerState", "testSmartHealth", "testPartitionEnumeration"};
}

std::vector<std::string> NvmeTesting::getPeriodicSafeTests() {
  return {"testControllerEnumeration", "testNamespaceEnumeration",
          "testControllerState", "testPartitionEnumeration"};
}

std::vector<std::string> NvmeTesting::getOneShotOnlyTests() {
  return {"testSmartHealth"};
}

std::vector<TestResult> NvmeTesting::runAllTests() {
  std::vector<TestResult> results;
  for (const auto &name : getAvailableTests())
    results.push_back(runSingleTest(name));
  return results;
}

TestResult NvmeTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testControllerEnumeration", &NvmeTesting::testControllerEnumeration},
      {"testNamespaceEnumeration", &NvmeTesting::testNamespaceEnumeration},
      {"testControllerState", &NvmeTesting::testControllerState},
      {"testSmartHealth", &NvmeTesting::testSmartHealth},
      {"testPartitionEnumeration", &NvmeTesting::testPartitionEnumeration},
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
