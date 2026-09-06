/**
 * @file mipi_csi_lib.cpp
 * @brief Implementation file for MipiCsiTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "mipi_csi_lib.h"
#include <iterator>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <vector>

namespace {

struct TestEntry {
    const char* name;
    TestOutput (MipiCsiTesting::*fn)();
};

std::string lowered(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

} // anonymous namespace

MipiCsiTesting::MipiCsiTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()), cmd_(std::make_shared<RealCommandProvider>()) {
}

MipiCsiTesting::~MipiCsiTesting() {}

MipiCsiTesting::MipiCsiTesting(std::shared_ptr<SysfsProvider> sysfs,
                               std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

TestOutput MipiCsiTesting::testVideoDeviceEnumeration() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    const auto entries = sysfs_->listDir("/sys/class/video4linux");
    if (entries.empty())
        return makeSkipped("no V4L2 devices present");
    TestOutput out = makePassed("V4L2 devices enumerated");
    addMetric(out, "v4l2_device_count", static_cast<int64_t>(entries.size()));
    return out;
}

TestOutput MipiCsiTesting::testCsiReceiverDetection() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    const auto entries = sysfs_->listDir("/sys/class/video4linux");
    for (const auto& dev : entries) {
        const std::string name =
            lowered(readSysfs(sysfs_, "/sys/class/video4linux/" + dev + "/name"));
        if (name.find("csi") != std::string::npos || name.find("unicam") != std::string::npos ||
            name.find("isp") != std::string::npos) {
            TestOutput out = makePassed("CSI receiver subdevice detected");
            addDetail(out, "device", dev);
            addDetail(out, "name", name);
            return out;
        }
    }
    return makeSkipped("no CSI receiver subdevice found");
}

TestOutput MipiCsiTesting::testSensorSubdeviceDetection() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    const auto entries = sysfs_->listDir("/sys/class/video4linux");
    for (const auto& dev : entries) {
        const std::string name =
            lowered(readSysfs(sysfs_, "/sys/class/video4linux/" + dev + "/name"));
        if (name.find("imx") == 0 || name.find("ov") == 0 ||
            name.find("sensor") != std::string::npos) {
            TestOutput out = makePassed("image sensor subdevice detected");
            addDetail(out, "device", dev);
            addDetail(out, "name", name);
            return out;
        }
    }
    return makeSkipped("no image sensor subdevice found");
}

TestOutput MipiCsiTesting::testMediaControllerGraph() {
    if (!cmd_)
        return makeSkipped("no command provider");
    const std::string out = runCommand(cmd_, "media-ctl -p 2>/dev/null");
    if (out.empty())
        return makeSkipped("media-ctl unavailable or no media controller");
    TestOutput o = makePassed("media controller graph dumped");
    addMetric(o, "graph_bytes", static_cast<double>(out.size()));
    return o;
}

TestOutput MipiCsiTesting::testV4l2DeviceNodes() {
    if (!cmd_)
        return makeSkipped("no command provider");
    const std::string out = runCommand(cmd_, "v4l2-ctl --list-devices 2>/dev/null");
    if (out.empty())
        return makeSkipped("v4l2-ctl unavailable");
    TestOutput o = makePassed("v4l2 device list retrieved");
    return o;
}

std::vector<std::string> MipiCsiTesting::getAvailableTests() {
    return {"testVideoDeviceEnumeration", "testCsiReceiverDetection",
            "testSensorSubdeviceDetection", "testMediaControllerGraph", "testV4l2DeviceNodes"};
}

std::vector<std::string> MipiCsiTesting::getPeriodicSafeTests() { return getAvailableTests(); }

std::vector<std::string> MipiCsiTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> MipiCsiTesting::runAllTests() {
    const std::vector<std::string> names = getAvailableTests();
    std::vector<TestResult> results;
    results.reserve(names.size());
    std::transform(names.begin(), names.end(), std::back_inserter(results),
                   [this](const std::string& name) { return runSingleTest(name); });
    return results;
}

TestResult MipiCsiTesting::runSingleTest(const std::string& testName) {
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput out;
    out.status = hal::Status::ERROR;
    out.severity = hal::Severity::ERROR;
    out.error_message = "Unknown test name: " + testName;

    const std::vector<TestEntry> table = {
        {"testVideoDeviceEnumeration", &MipiCsiTesting::testVideoDeviceEnumeration},
        {"testCsiReceiverDetection", &MipiCsiTesting::testCsiReceiverDetection},
        {"testSensorSubdeviceDetection", &MipiCsiTesting::testSensorSubdeviceDetection},
        {"testMediaControllerGraph", &MipiCsiTesting::testMediaControllerGraph},
        {"testV4l2DeviceNodes", &MipiCsiTesting::testV4l2DeviceNodes},
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
