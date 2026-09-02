/**
 * @file pwm_output_lib.cpp
 * @brief Implementation file for PwmOutputTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "pwm_output_lib.h"

#include <algorithm>
#include <chrono>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (PwmOutputTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

} // anonymous namespace

PwmOutputTesting::PwmOutputTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

PwmOutputTesting::~PwmOutputTesting() {}

PwmOutputTesting::PwmOutputTesting(std::shared_ptr<SysfsProvider> sysfs,
                                   std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string PwmOutputTesting::firstChipDir() const {
  if (!sysfs_)
    return "";
  for (const auto &e : sysfs_->listDir("/sys/class/pwm")) {
    if (hasPrefix(e, "pwmchip"))
      return "/sys/class/pwm/" + e;
  }
  return "";
}

std::string PwmOutputTesting::exportChannel0(const std::string &chipDir) const {
  if (!sysfs_)
    return "";
  if (!sysfs_->writeFile(chipDir + "/export", "0"))
    return "";
  // After export the chip directory gains a "pwm0" entry.
  const auto entries = sysfs_->listDir(chipDir);
  if (std::find(entries.begin(), entries.end(), "pwm0") != entries.end())
    return chipDir + "/pwm0";
  return "";
}

void PwmOutputTesting::unexportChannel0(const std::string &chipDir) const {
  if (sysfs_)
    sysfs_->writeFile(chipDir + "/unexport", "0");
}

TestOutput PwmOutputTesting::testPwmChipEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  long long count = 0;
  for (const auto &e : sysfs_->listDir("/sys/class/pwm")) {
    if (hasPrefix(e, "pwmchip"))
      ++count;
  }
  if (count == 0)
    return makeSkipped("no PWM chips present");
  TestOutput out = makePassed("PWM chips enumerated");
  addMetric(out, "chip_count", count);
  return out;
}

TestOutput PwmOutputTesting::testChannelCount() {
  const std::string chipDir = firstChipDir();
  if (chipDir.empty())
    return makeSkipped("no PWM chips present");
  const long long npwm = readSysfsInt(sysfs_, chipDir + "/npwm");
  if (npwm <= 0)
    return makeFailed("npwm attribute missing or zero");
  TestOutput out = makePassed("channel count read");
  addMetric(out, "npwm", static_cast<double>(npwm));
  addDetail(out, "chip", chipDir);
  return out;
}

TestOutput PwmOutputTesting::testExportUnexport() {
  const std::string chipDir = firstChipDir();
  if (chipDir.empty())
    return makeSkipped("no PWM chips present");
  const long long npwm = readSysfsInt(sysfs_, chipDir + "/npwm");
  if (npwm <= 0)
    return makeSkipped("chip exposes no channels");
  const std::string pwmNode = exportChannel0(chipDir);
  if (pwmNode.empty())
    return makeFailed("failed to export PWM channel 0");
  unexportChannel0(chipDir);
  TestOutput out = makePassed("export/unexport cycle completed");
  addDetail(out, "chip", chipDir);
  return out;
}

TestOutput PwmOutputTesting::testPeriodDutyConfig() {
  const std::string chipDir = firstChipDir();
  if (chipDir.empty())
    return makeSkipped("no PWM chips present");
  if (readSysfsInt(sysfs_, chipDir + "/npwm") <= 0)
    return makeSkipped("chip exposes no channels");
  const std::string pwmNode = exportChannel0(chipDir);
  if (pwmNode.empty())
    return makeSkipped("failed to export PWM channel 0");
  // 1 kHz, 0 % duty — safe idle-low output.
  const std::string period = std::to_string(1000000);
  const std::string duty = std::to_string(0);
  bool ok = sysfs_->writeFile(pwmNode + "/period", period) &&
            sysfs_->writeFile(pwmNode + "/duty_cycle", duty);
  long long periodRead = ok ? readSysfsInt(sysfs_, pwmNode + "/period") : -1;
  unexportChannel0(chipDir);
  if (!ok)
    return makeFailed("failed to write period/duty_cycle");
  if (periodRead != 1000000)
    return makeFailed("period readback mismatch: " +
                      std::to_string(periodRead));
  TestOutput out = makePassed("period/duty configured and verified");
  addMetric(out, "period_ns", 1000000.0);
  addMetric(out, "duty_cycle_ns", 0.0);
  return out;
}

TestOutput PwmOutputTesting::testEnableDisable() {
  const std::string chipDir = firstChipDir();
  if (chipDir.empty())
    return makeSkipped("no PWM chips present");
  if (readSysfsInt(sysfs_, chipDir + "/npwm") <= 0)
    return makeSkipped("chip exposes no channels");
  const std::string pwmNode = exportChannel0(chipDir);
  if (pwmNode.empty())
    return makeSkipped("failed to export PWM channel 0");
  // Configure a safe idle-low output before enabling.
  sysfs_->writeFile(pwmNode + "/period", "1000000");
  sysfs_->writeFile(pwmNode + "/duty_cycle", "0");
  bool ok = sysfs_->writeFile(pwmNode + "/enable", "1");
  const long long enabledRead =
      ok ? readSysfsInt(sysfs_, pwmNode + "/enable") : -1;
  sysfs_->writeFile(pwmNode + "/enable", "0");
  unexportChannel0(chipDir);
  if (!ok)
    return makeFailed("failed to enable PWM channel");
  if (enabledRead != 1)
    return makeFailed("enable readback mismatch: " +
                      std::to_string(enabledRead));
  TestOutput out = makePassed("enable/disable cycle verified");
  return out;
}

std::vector<std::string> PwmOutputTesting::getAvailableTests() {
  return {"testPwmChipEnumeration", "testChannelCount", "testExportUnexport",
          "testPeriodDutyConfig", "testEnableDisable"};
}

std::vector<std::string> PwmOutputTesting::getPeriodicSafeTests() {
  return {"testPwmChipEnumeration", "testChannelCount"};
}

std::vector<std::string> PwmOutputTesting::getOneShotOnlyTests() {
  return {"testExportUnexport", "testPeriodDutyConfig", "testEnableDisable"};
}

std::vector<TestResult> PwmOutputTesting::runAllTests() {
  std::vector<TestResult> results;
  for (const auto &name : getAvailableTests())
    results.push_back(runSingleTest(name));
  return results;
}

TestResult PwmOutputTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testPwmChipEnumeration", &PwmOutputTesting::testPwmChipEnumeration},
      {"testChannelCount", &PwmOutputTesting::testChannelCount},
      {"testExportUnexport", &PwmOutputTesting::testExportUnexport},
      {"testPeriodDutyConfig", &PwmOutputTesting::testPeriodDutyConfig},
      {"testEnableDisable", &PwmOutputTesting::testEnableDisable},
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
