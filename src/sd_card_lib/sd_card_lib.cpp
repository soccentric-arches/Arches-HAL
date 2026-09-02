/**
 * @file sd_card_lib.cpp
 * @brief Implementation file for SdCardTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "sd_card_lib.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (SdCardTesting::*fn)();
};

/// Find the first SD-type card dir under /sys/class/mmc_host/<host>/<card>.
std::string findSdCardDir(const std::shared_ptr<SysfsProvider> &sysfs) {
  if (!sysfs)
    return "";
  for (const auto &host : sysfs->listDir("/sys/class/mmc_host")) {
    const std::string hostDir = "/sys/class/mmc_host/" + host;
    for (const auto &card : sysfs->listDir(hostDir)) {
      const std::string type = readSysfs(sysfs, hostDir + "/" + card + "/type");
      if (type == "SD" || type == "SDIO" || type == "SD combo")
        return hostDir + "/" + card;
    }
  }
  return "";
}

bool allHexDigits(const std::string &s) {
  if (s.empty())
    return false;
  for (const char c : s) {
    if (!std::isxdigit(static_cast<unsigned char>(c)))
      return false;
  }
  return true;
}

} // anonymous namespace

SdCardTesting::SdCardTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

SdCardTesting::~SdCardTesting() {}

SdCardTesting::SdCardTesting(std::shared_ptr<SysfsProvider> sysfs,
                             std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string SdCardTesting::firstSdCardDir() const {
  return findSdCardDir(sysfs_);
}

TestOutput SdCardTesting::testMmcHostEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/class/mmc_host");
  if (entries.empty())
    return makeSkipped("no MMC hosts present");
  TestOutput out = makePassed("MMC hosts enumerated");
  addMetric(out, "host_count", static_cast<int64_t>(entries.size()));
  return out;
}

TestOutput SdCardTesting::testSdCardPresence() {
  const std::string card = firstSdCardDir();
  if (card.empty())
    return makeSkipped("no SD-type card detected");
  TestOutput out = makePassed("SD card detected");
  addDetail(out, "sysfs_path", card);
  return out;
}

TestOutput SdCardTesting::testCidRead() {
  const std::string card = firstSdCardDir();
  if (card.empty())
    return makeSkipped("no SD card detected");
  const std::string cid = readSysfs(sysfs_, card + "/cid");
  if (cid.empty())
    return makeSkipped("cid attribute unavailable");
  if (!allHexDigits(cid))
    return makeFailed("cid is not a hex string: " + cid);
  TestOutput out = makePassed("CID read");
  addDetail(out, "cid", cid);
  return out;
}

TestOutput SdCardTesting::testBlockDeviceSize() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const std::string card = firstSdCardDir();
  if (card.empty())
    return makeSkipped("no SD card detected");
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
  return out;
}

TestOutput SdCardTesting::testWriteProtectState() {
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
  const long long ro = readSysfsInt(sysfs_, "/sys/block/" + mmcblk + "/ro");
  if (ro < 0)
    return makeSkipped("ro attribute unavailable");
  TestOutput out = makePassed(ro == 0 ? "card is writable"
                                      : "card is read-only (write protected)");
  addMetric(out, "read_only", ro != 0);
  return out;
}

std::vector<std::string> SdCardTesting::getAvailableTests() {
  return {"testMmcHostEnumeration", "testSdCardPresence", "testCidRead",
          "testBlockDeviceSize", "testWriteProtectState"};
}

std::vector<std::string> SdCardTesting::getPeriodicSafeTests() {
  return getAvailableTests();
}

std::vector<std::string> SdCardTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> SdCardTesting::runAllTests() {
  std::vector<TestResult> results;
  for (const auto &name : getAvailableTests())
    results.push_back(runSingleTest(name));
  return results;
}

TestResult SdCardTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testMmcHostEnumeration", &SdCardTesting::testMmcHostEnumeration},
      {"testSdCardPresence", &SdCardTesting::testSdCardPresence},
      {"testCidRead", &SdCardTesting::testCidRead},
      {"testBlockDeviceSize", &SdCardTesting::testBlockDeviceSize},
      {"testWriteProtectState", &SdCardTesting::testWriteProtectState},
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
