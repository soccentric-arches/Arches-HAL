/**
 * @file analog_input_lib.cpp
 * @brief Implementation file for AnalogInputTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include <iterator>
#include "analog_input_lib.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (AnalogInputTesting::*fn)();
};

bool hasPrefix(const std::string &s, const std::string &prefix) {
  return s.rfind(prefix, 0) == 0;
}

} // anonymous namespace

AnalogInputTesting::AnalogInputTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

AnalogInputTesting::~AnalogInputTesting() {}

AnalogInputTesting::AnalogInputTesting(std::shared_ptr<SysfsProvider> sysfs,
                                       std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string AnalogInputTesting::firstAdcDeviceDir() const {
  if (!sysfs_)
    return "";
  for (const auto &dev : sysfs_->listDir("/sys/bus/iio/devices")) {
    if (!hasPrefix(dev, "iio:device"))
      continue;
    const std::string devDir = "/sys/bus/iio/devices/" + dev;
    for (const auto &f : sysfs_->listDir(devDir)) {
      if (hasPrefix(f, "in_voltage") && f.size() > 4 &&
          f.compare(f.size() - 4, 4, "_raw") == 0)
        return devDir;
    }
  }
  return "";
}

TestOutput AnalogInputTesting::testIioDeviceEnumeration() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  long long count = 0;
  for (const auto &e : sysfs_->listDir("/sys/bus/iio/devices")) {
    if (hasPrefix(e, "iio:device"))
      ++count;
  }
  if (count == 0)
    return makeSkipped("no IIO devices present");
  TestOutput out = makePassed("IIO devices enumerated");
  addMetric(out, "iio_device_count", count);
  return out;
}

TestOutput AnalogInputTesting::testAdcChannelDetection() {
  const std::string devDir = firstAdcDeviceDir();
  if (devDir.empty())
    return makeSkipped("no IIO device with ADC voltage channels found");
  long long channels = 0;
  for (const auto &f : sysfs_->listDir(devDir)) {
    if (hasPrefix(f, "in_voltage") && f.size() > 4 &&
        f.compare(f.size() - 4, 4, "_raw") == 0)
      ++channels;
  }
  TestOutput out = makePassed("ADC voltage channels detected");
  addMetric(out, "channel_count", channels);
  addDetail(out, "device", devDir);
  return out;
}

TestOutput AnalogInputTesting::testRawRead() {
  const std::string devDir = firstAdcDeviceDir();
  if (devDir.empty())
    return makeSkipped("no IIO device with ADC voltage channels found");
  std::string rawFile;
  for (const auto &f : sysfs_->listDir(devDir)) {
    if (hasPrefix(f, "in_voltage") && f.size() > 4 &&
        f.compare(f.size() - 4, 4, "_raw") == 0) {
      rawFile = f;
      break;
    }
  }
  if (rawFile.empty())
    return makeSkipped("no voltage channel file");
  const long long raw = readSysfsInt(sysfs_, devDir + "/" + rawFile);
  if (raw < 0)
    return makeFailed("raw read failed for " + rawFile);
  TestOutput out = makePassed("raw ADC sample read");
  addMetric(out, "raw", static_cast<int64_t>(raw));
  addDetail(out, "channel_file", rawFile);
  return out;
}

TestOutput AnalogInputTesting::testScaleRead() {
  const std::string devDir = firstAdcDeviceDir();
  if (devDir.empty())
    return makeSkipped("no IIO device with ADC voltage channels found");
  const std::string scaleStr = readSysfs(sysfs_, devDir + "/in_voltage_scale");
  if (scaleStr.empty())
    return makeSkipped("in_voltage_scale unavailable");
  double scaleVal = -1.0;
  try {
    scaleVal = std::stod(scaleStr);
  } catch (...) {
    scaleVal = -1.0;
  }
  if (scaleVal <= 0)
    return makeFailed("invalid scale value: " + scaleStr);
  TestOutput out = makePassed("scale read");
  addMetric(out, "scale", scaleVal);
  return out;
}

TestOutput AnalogInputTesting::testSaturationCheck() {
  const std::string devDir = firstAdcDeviceDir();
  if (devDir.empty())
    return makeSkipped("no IIO device with ADC voltage channels found");
  std::string rawFile;
  for (const auto &f : sysfs_->listDir(devDir)) {
    if (hasPrefix(f, "in_voltage") && f.size() > 4 &&
        f.compare(f.size() - 4, 4, "_raw") == 0) {
      rawFile = f;
      break;
    }
  }
  if (rawFile.empty())
    return makeSkipped("no voltage channel file");
  const int kSamples = 10;
  long long minVal = -1, maxVal = -1;
  long long firstVal = -1;
  bool allSame = true;
  for (int i = 0; i < kSamples; ++i) {
    const long long raw = readSysfsInt(sysfs_, devDir + "/" + rawFile);
    if (raw < 0)
      return makeSkipped("raw read failed during sampling");
    if (i == 0) {
      minVal = raw;
      maxVal = raw;
      firstVal = raw;
    } else {
      minVal = std::min(minVal, raw);
      maxVal = std::max(maxVal, raw);
      if (raw != firstVal)
        allSame = false;
    }
    if (i + 1 < kSamples)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  // A channel pinned at a rail (common full-scale codes) that never moves
  // suggests a wiring or configuration fault rather than a healthy input.
  const bool railing = allSame && (firstVal == 0 || firstVal == 4095 ||
                                   firstVal == 1023 || firstVal == 65535);
  TestOutput out = makePassed(railing ? "samples constant at rail — check input"
                                      : "samples collected");
  if (railing) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
  }
  addMetric(out, "samples_min", static_cast<int64_t>(minVal));
  addMetric(out, "samples_max", static_cast<int64_t>(maxVal));
  return out;
}

std::vector<std::string> AnalogInputTesting::getAvailableTests() {
  return {"testIioDeviceEnumeration", "testAdcChannelDetection", "testRawRead",
          "testScaleRead", "testSaturationCheck"};
}

std::vector<std::string> AnalogInputTesting::getPeriodicSafeTests() {
  return getAvailableTests();
}

std::vector<std::string> AnalogInputTesting::getOneShotOnlyTests() {
  return {};
}

std::vector<TestResult> AnalogInputTesting::runAllTests() {
  const std::vector<std::string> names = getAvailableTests();
  std::vector<TestResult> results;
  results.reserve(names.size());
  std::transform(names.begin(), names.end(), std::back_inserter(results),
                 [this](const std::string &name) { return runSingleTest(name); });
  return results;
}

TestResult AnalogInputTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testIioDeviceEnumeration",
       &AnalogInputTesting::testIioDeviceEnumeration},
      {"testAdcChannelDetection", &AnalogInputTesting::testAdcChannelDetection},
      {"testRawRead", &AnalogInputTesting::testRawRead},
      {"testScaleRead", &AnalogInputTesting::testScaleRead},
      {"testSaturationCheck", &AnalogInputTesting::testSaturationCheck},
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
