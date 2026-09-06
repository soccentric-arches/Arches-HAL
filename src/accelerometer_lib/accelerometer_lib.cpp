/**
 * @file accelerometer_lib.cpp
 * @brief Implementation file for AccelerometerTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "accelerometer_lib.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iterator>
#include <vector>

namespace {

struct TestEntry {
    const char* name;
    TestOutput (AccelerometerTesting::*fn)();
};

bool hasPrefix(const std::string& s, const std::string& prefix) { return s.rfind(prefix, 0) == 0; }

} // anonymous namespace

AccelerometerTesting::AccelerometerTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()), cmd_(std::make_shared<RealCommandProvider>()) {
}

AccelerometerTesting::~AccelerometerTesting() {}

AccelerometerTesting::AccelerometerTesting(std::shared_ptr<SysfsProvider> sysfs,
                                           std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

std::string AccelerometerTesting::firstAccelDeviceDir() const {
    if (!sysfs_)
        return "";
    const std::vector<std::string> devices = sysfs_->listDir("/sys/bus/iio/devices");
    const auto it = std::find_if(devices.begin(), devices.end(), [this](const std::string& dev) {
        if (!hasPrefix(dev, "iio:device"))
            return false;
        const std::vector<std::string> files = sysfs_->listDir("/sys/bus/iio/devices/" + dev);
        return std::find(files.begin(), files.end(), "in_accel_x_raw") != files.end();
    });
    if (it == devices.end())
        return "";
    return "/sys/bus/iio/devices/" + *it;
}

TestOutput AccelerometerTesting::testAccelDeviceDetection() {
    const std::string devDir = firstAccelDeviceDir();
    if (devDir.empty())
        return makeSkipped("no IIO accelerometer (in_accel_x_raw) found");
    TestOutput out = makePassed("accelerometer device detected");
    addDetail(out, "sysfs_path", devDir);
    return out;
}

TestOutput AccelerometerTesting::testAxisReads() {
    const std::string devDir = firstAccelDeviceDir();
    if (devDir.empty())
        return makeSkipped("no IIO accelerometer device found");
    const long long x = readSysfsInt(sysfs_, devDir + "/in_accel_x_raw");
    const long long y = readSysfsInt(sysfs_, devDir + "/in_accel_y_raw");
    const long long z = readSysfsInt(sysfs_, devDir + "/in_accel_z_raw");
    if (x < 0 || y < 0 || z < 0)
        return makeFailed("one or more axis reads failed");
    TestOutput out = makePassed("axis samples read");
    addMetric(out, "accel_x_raw", static_cast<int64_t>(x));
    addMetric(out, "accel_y_raw", static_cast<int64_t>(y));
    addMetric(out, "accel_z_raw", static_cast<int64_t>(z));
    return out;
}

TestOutput AccelerometerTesting::testScaleRead() {
    const std::string devDir = firstAccelDeviceDir();
    if (devDir.empty())
        return makeSkipped("no IIO accelerometer device found");
    const std::string scaleStr = readSysfs(sysfs_, devDir + "/in_accel_scale");
    if (scaleStr.empty())
        return makeSkipped("in_accel_scale unavailable");
    double scale = -1.0;
    try {
        scale = std::stod(scaleStr);
    } catch (...) {
        scale = -1.0;
    }
    if (scale <= 0)
        return makeFailed("invalid scale value: " + scaleStr);
    TestOutput out = makePassed("scale read");
    addMetric(out, "scale_g_per_lsb", scale);
    return out;
}

TestOutput AccelerometerTesting::testSamplingFrequency() {
    const std::string devDir = firstAccelDeviceDir();
    if (devDir.empty())
        return makeSkipped("no IIO accelerometer device found");
    const std::string freqStr = readSysfs(sysfs_, devDir + "/in_accel_sampling_frequency");
    if (freqStr.empty())
        return makeSkipped("in_accel_sampling_frequency unavailable");
    double freq = -1.0;
    try {
        freq = std::stod(freqStr);
    } catch (...) {
        freq = -1.0;
    }
    if (freq <= 0)
        return makeFailed("invalid sampling frequency: " + freqStr);
    TestOutput out = makePassed("sampling frequency read");
    addMetric(out, "sampling_frequency_Hz", freq);
    return out;
}

TestOutput AccelerometerTesting::testMagnitudeSanity() {
    const std::string devDir = firstAccelDeviceDir();
    if (devDir.empty())
        return makeSkipped("no IIO accelerometer device found");
    const long long x = readSysfsInt(sysfs_, devDir + "/in_accel_x_raw");
    const long long y = readSysfsInt(sysfs_, devDir + "/in_accel_y_raw");
    const long long z = readSysfsInt(sysfs_, devDir + "/in_accel_z_raw");
    if (x < 0 || y < 0 || z < 0)
        return makeSkipped("axis reads failed");
    double scale = 1.0;
    const std::string scaleStr = readSysfs(sysfs_, devDir + "/in_accel_scale");
    if (!scaleStr.empty()) {
        try {
            scale = std::stod(scaleStr);
        } catch (...) {
            scale = 1.0;
        }
    }
    // Magnitude in g (raw is scaled by in_accel_scale = g per LSB).
    const double gx = static_cast<double>(x) * scale;
    const double gy = static_cast<double>(y) * scale;
    const double gz = static_cast<double>(z) * scale;
    const double magnitudeG = std::sqrt(gx * gx + gy * gy + gz * gz);
    // A static sensor should read roughly 1 g (allow 0.2-2 g for mounting
    // orientation, offsets, and coarse scale rounding).
    const bool sane = magnitudeG >= 0.2 && magnitudeG <= 2.0;
    TestOutput out = makePassed(sane ? "static magnitude within expected range"
                                     : "static magnitude out of expected range");
    if (!sane) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::ERROR;
    }
    addMetric(out, "magnitude_g", magnitudeG);
    return out;
}

std::vector<std::string> AccelerometerTesting::getAvailableTests() {
    return {"testAccelDeviceDetection", "testAxisReads", "testScaleRead", "testSamplingFrequency",
            "testMagnitudeSanity"};
}

std::vector<std::string> AccelerometerTesting::getPeriodicSafeTests() {
    return getAvailableTests();
}

std::vector<std::string> AccelerometerTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> AccelerometerTesting::runAllTests() {
    const std::vector<std::string> names = getAvailableTests();
    std::vector<TestResult> results;
    results.reserve(names.size());
    std::transform(names.begin(), names.end(), std::back_inserter(results),
                   [this](const std::string& name) { return runSingleTest(name); });
    return results;
}

TestResult AccelerometerTesting::runSingleTest(const std::string& testName) {
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput out;
    out.status = hal::Status::ERROR;
    out.severity = hal::Severity::ERROR;
    out.error_message = "Unknown test name: " + testName;

    const std::vector<TestEntry> table = {
        {"testAccelDeviceDetection", &AccelerometerTesting::testAccelDeviceDetection},
        {"testAxisReads", &AccelerometerTesting::testAxisReads},
        {"testScaleRead", &AccelerometerTesting::testScaleRead},
        {"testSamplingFrequency", &AccelerometerTesting::testSamplingFrequency},
        {"testMagnitudeSanity", &AccelerometerTesting::testMagnitudeSanity},
    };
    const auto entry = std::find_if(table.begin(), table.end(),
                                    [&testName](const TestEntry& e) { return testName == e.name; });
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
