/**
 * @file emmc_lib.cpp
 * @brief Implementation file for EmmcTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include <iterator>
#include "emmc_lib.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (EmmcTesting::*fn)();
};

/// Find the first MMC-type card dir, e.g. /sys/class/mmc_host/mmc0/mmc0:0001.
std::string findCardDirOfType(const std::shared_ptr<SysfsProvider> &sysfs,
                              const std::string &wantedType) {
  if (!sysfs)
    return "";
  for (const auto &host : sysfs->listDir("/sys/class/mmc_host")) {
    const std::string hostDir = "/sys/class/mmc_host/" + host;
    for (const auto &card : sysfs->listDir(hostDir)) {
      const std::string type = readSysfs(sysfs, hostDir + "/" + card + "/type");
      if (type == wantedType)
        return hostDir + "/" + card;
    }
  }
  return "";
}

} // anonymous namespace

EmmcTesting::EmmcTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

EmmcTesting::~EmmcTesting() {}

EmmcTesting::EmmcTesting(std::shared_ptr<SysfsProvider> sysfs,
                         std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string EmmcTesting::firstEmmcCardDir() const {
  return findCardDirOfType(sysfs_, "MMC");
}

TestOutput EmmcTesting::testMmcHostEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/class/mmc_host");
  if (entries.empty())
    return makeSkipped("no MMC hosts present");
  TestOutput out = makePassed("MMC hosts enumerated");
  addMetric(out, "host_count", static_cast<int64_t>(entries.size()));
  return out;
}

TestOutput EmmcTesting::testEmmcDevicePresence() {
  const std::string card = firstEmmcCardDir();
  if (card.empty())
    return makeSkipped("no eMMC (MMC-type) card detected");
  TestOutput out = makePassed("eMMC card detected");
  addDetail(out, "sysfs_path", card);
  return out;
}

TestOutput EmmcTesting::testDeviceNameRead() {
  const std::string card = firstEmmcCardDir();
  if (card.empty())
    return makeSkipped("no eMMC card detected");
  const std::string name = readSysfs(sysfs_, card + "/name");
  if (name.empty())
    return makeFailed("eMMC name attribute missing or empty");
  TestOutput out = makePassed("eMMC device name read");
  addDetail(out, "name", name);
  return out;
}

TestOutput EmmcTesting::testLifeTimeHealth() {
  const std::string card = firstEmmcCardDir();
  if (card.empty())
    return makeSkipped("no eMMC card detected");
  // life_time format: "0x0A 0x0A" (estimate type A and B, 1-10, 11 = exceeded).
  const std::string lifeTime = readSysfs(sysfs_, card + "/life_time");
  if (lifeTime.empty())
    return makeSkipped("life_time attribute unavailable");
  long long a = -1, b = -1;
  {
    std::istringstream ss(lifeTime);
    std::string tok;
    int idx = 0;
    while (ss >> tok && idx < 2) {
      try {
        if (idx == 0)
          a = std::stoll(tok, nullptr, 0);
        else
          b = std::stoll(tok, nullptr, 0);
        ++idx;
      } catch (...) {
        break;
      }
    }
  }
  if (a <= 0)
    return makeSkipped("life_time value unreadable");
  const bool exceeded = (a > 10) || (b > 10);
  TestOutput out = makePassed(exceeded ? "eMMC life time estimate exceeded"
                                       : "eMMC life time within spec");
  if (exceeded) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
  }
  addMetric(out, "life_time_a", static_cast<int64_t>(a));
  if (b > 0)
    addMetric(out, "life_time_b", static_cast<int64_t>(b));
  return out;
}

TestOutput EmmcTesting::testPreEolInfo() {
  const std::string card = firstEmmcCardDir();
  if (card.empty())
    return makeSkipped("no eMMC card detected");
  const std::string eol = readSysfs(sysfs_, card + "/pre_eol_info");
  if (eol.empty())
    return makeSkipped("pre_eol_info attribute unavailable");
  long long val = -1;
  try {
    val = std::stoll(eol, nullptr, 0);
  } catch (...) {
    val = -1;
  }
  if (val < 0)
    return makeSkipped("pre_eol_info value unparseable: " + eol);
  if (val == 0x01) {
    TestOutput out = makePassed("eMMC pre-EOL status normal");
    addMetric(out, "pre_eol_info", static_cast<int64_t>(val));
    return out;
  }
  const char *desc = (val == 0x02) ? "warning (reserved)"
                                   : ((val == 0x03) ? "urgent" : "unknown");
  TestOutput out = makeFailed("eMMC pre-EOL status: " + std::string(desc));
  addMetric(out, "pre_eol_info", static_cast<double>(val));
  return out;
}

TestOutput EmmcTesting::testBlockDeviceSize() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto blocks = sysfs_->listDir("/sys/block");
  std::string mmcblk;
  for (const auto &b : blocks) {
    if (b.rfind("mmcblk", 0) == 0 && b.find('p') == std::string::npos) {
      mmcblk = b;
      break;
    }
  }
  if (mmcblk.empty())
    return makeSkipped("no mmcblk block device present");
  const long long sectors =
      readSysfsInt(sysfs_, "/sys/block/" + mmcblk + "/size");
  if (sectors <= 0)
    return makeFailed("block size attribute missing or invalid");
  TestOutput out = makePassed("block device size read");
  addMetric(out, "size_512B_sectors", static_cast<double>(sectors));
  addMetric(out, "size_GB", static_cast<double>(sectors) * 512.0 / 1e9);
  addDetail(out, "device", mmcblk);
  return out;
}

std::vector<std::string> EmmcTesting::getAvailableTests() {
  return {"testMmcHostEnumeration", "testEmmcDevicePresence",
          "testDeviceNameRead",     "testLifeTimeHealth",
          "testPreEolInfo",         "testBlockDeviceSize"};
}

std::vector<std::string> EmmcTesting::getPeriodicSafeTests() {
  return getAvailableTests();
}

std::vector<std::string> EmmcTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> EmmcTesting::runAllTests() {
  const std::vector<std::string> names = getAvailableTests();
  std::vector<TestResult> results;
  results.reserve(names.size());
  std::transform(names.begin(), names.end(), std::back_inserter(results),
                 [this](const std::string &name) { return runSingleTest(name); });
  return results;
}

TestResult EmmcTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testMmcHostEnumeration", &EmmcTesting::testMmcHostEnumeration},
      {"testEmmcDevicePresence", &EmmcTesting::testEmmcDevicePresence},
      {"testDeviceNameRead", &EmmcTesting::testDeviceNameRead},
      {"testLifeTimeHealth", &EmmcTesting::testLifeTimeHealth},
      {"testPreEolInfo", &EmmcTesting::testPreEolInfo},
      {"testBlockDeviceSize", &EmmcTesting::testBlockDeviceSize},
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
