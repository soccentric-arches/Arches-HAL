/**
 * @file mipi_dsi_lib.cpp
 * @brief Implementation file for MipiDsiTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "mipi_dsi_lib.h"
#include <iterator>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <vector>

namespace {

struct TestEntry {
    const char* name;
    TestOutput (MipiDsiTesting::*fn)();
};

bool hasPrefix(const std::string& s, const std::string& prefix) { return s.rfind(prefix, 0) == 0; }

/// True if the DRM entry is a connector whose name contains "DSI".
bool isDsiConnector(const std::string& entry) {
    // Connector entries look like card0-DSI-1, card0-HDMI-A-1, card0-eDP-1.
    const std::size_t dash = entry.find('-');
    if (dash == std::string::npos)
        return false;
    const std::string connector = entry.substr(dash + 1);
    return connector.rfind("DSI", 0) == 0;
}

} // anonymous namespace

MipiDsiTesting::MipiDsiTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()), cmd_(std::make_shared<RealCommandProvider>()) {
}

MipiDsiTesting::~MipiDsiTesting() {}

MipiDsiTesting::MipiDsiTesting(std::shared_ptr<SysfsProvider> sysfs,
                               std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

TestOutput MipiDsiTesting::testDrmCardEnumeration() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    long long count = 0;
    for (const auto& e : sysfs_->listDir("/sys/class/drm")) {
        if (hasPrefix(e, "card") && e.find('-') == std::string::npos)
            ++count;
    }
    if (count == 0)
        return makeSkipped("no DRM cards present");
    TestOutput out = makePassed("DRM cards enumerated");
    addMetric(out, "card_count", count);
    return out;
}

TestOutput MipiDsiTesting::testDsiConnectorDetection() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    long long count = 0;
    std::string first;
    for (const auto& e : sysfs_->listDir("/sys/class/drm")) {
        if (isDsiConnector(e)) {
            if (first.empty())
                first = e;
            ++count;
        }
    }
    if (count == 0)
        return makeSkipped("no DSI connectors exposed by the DRM subsystem");
    TestOutput out = makePassed("DSI connector(s) detected");
    addMetric(out, "dsi_connector_count", count);
    addDetail(out, "first_connector", first);
    return out;
}

TestOutput MipiDsiTesting::testConnectorStatus() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    std::string connector;
    for (const auto& e : sysfs_->listDir("/sys/class/drm")) {
        if (isDsiConnector(e)) {
            connector = e;
            break;
        }
    }
    if (connector.empty())
        return makeSkipped("no DSI connectors present");
    const std::string status = readSysfs(sysfs_, "/sys/class/drm/" + connector + "/status");
    if (status.empty())
        return makeFailed("connector status attribute missing");
    TestOutput out = makePassed("connector status read");
    addDetail(out, "connector", connector);
    addDetail(out, "status", status);
    return out;
}

TestOutput MipiDsiTesting::testBacklightEnumeration() {
    if (!sysfs_)
        return makeSkipped("no sysfs provider");
    const auto entries = sysfs_->listDir("/sys/class/backlight");
    if (entries.empty())
        return makeSkipped("no backlight devices present");
    TestOutput out = makePassed("backlight devices enumerated");
    addMetric(out, "backlight_count", static_cast<int64_t>(entries.size()));
    return out;
}

TestOutput MipiDsiTesting::testDsiHostDriverBinding() {
    if (!sysfs_ || !cmd_)
        return makeSkipped("no providers");
    std::string connector;
    for (const auto& e : sysfs_->listDir("/sys/class/drm")) {
        if (isDsiConnector(e)) {
            connector = e;
            break;
        }
    }
    if (connector.empty())
        return makeSkipped("no DSI connectors present");
    if (!isSafeName(connector))
        return makeSkipped("suspicious connector name, refusing to run readlink");
    const std::string link =
        runCommand(cmd_, "readlink -f /sys/class/drm/" + connector + "/device/driver 2>/dev/null");
    if (link.empty())
        return makeSkipped("no driver bound to " + connector);
    const std::string driver = link.substr(link.find_last_of('/') + 1);
    TestOutput out = makePassed("DSI host driver resolved");
    addDetail(out, "connector", connector);
    addDetail(out, "driver", driver);
    return out;
}

std::vector<std::string> MipiDsiTesting::getAvailableTests() {
    return {"testDrmCardEnumeration", "testDsiConnectorDetection", "testConnectorStatus",
            "testBacklightEnumeration", "testDsiHostDriverBinding"};
}

std::vector<std::string> MipiDsiTesting::getPeriodicSafeTests() { return getAvailableTests(); }

std::vector<std::string> MipiDsiTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> MipiDsiTesting::runAllTests() {
    const std::vector<std::string> names = getAvailableTests();
    std::vector<TestResult> results;
    results.reserve(names.size());
    std::transform(names.begin(), names.end(), std::back_inserter(results),
                   [this](const std::string& name) { return runSingleTest(name); });
    return results;
}

TestResult MipiDsiTesting::runSingleTest(const std::string& testName) {
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput out;
    out.status = hal::Status::ERROR;
    out.severity = hal::Severity::ERROR;
    out.error_message = "Unknown test name: " + testName;

    const std::vector<TestEntry> table = {
        {"testDrmCardEnumeration", &MipiDsiTesting::testDrmCardEnumeration},
        {"testDsiConnectorDetection", &MipiDsiTesting::testDsiConnectorDetection},
        {"testConnectorStatus", &MipiDsiTesting::testConnectorStatus},
        {"testBacklightEnumeration", &MipiDsiTesting::testBacklightEnumeration},
        {"testDsiHostDriverBinding", &MipiDsiTesting::testDsiHostDriverBinding},
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
