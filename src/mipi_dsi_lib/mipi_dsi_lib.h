/**
 * @file mipi_dsi_lib.h
 * @brief Header file for MipiDsiTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef MIPIDSITESTING_H
#define MIPIDSITESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class MipiDsiTesting : public TestBase
 * @brief Class for testing MIPI DSI display interface functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class MipiDsiTesting : public TestBase {
    public:
    MipiDsiTesting();
    ~MipiDsiTesting();

    explicit MipiDsiTesting(std::shared_ptr<SysfsProvider> sysfs,
                            std::shared_ptr<CommandProvider> cmd = nullptr);

    /// Enumerate DRM cards in /sys/class/drm.
    TestOutput testDrmCardEnumeration();
    /// Detect DSI connectors (cardN-DSI-*).
    TestOutput testDsiConnectorDetection();
    /// Read connector status (connected/disconnected).
    TestOutput testConnectorStatus();
    /// Enumerate backlight devices.
    TestOutput testBacklightEnumeration();
    /// Resolve the DSI host driver binding.
    TestOutput testDsiHostDriverBinding();

    std::vector<std::string> getAvailableTests() override;
    std::vector<std::string> getPeriodicSafeTests();
    std::vector<std::string> getOneShotOnlyTests();

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;
    TestResult runSingleTest(const std::string& testName) override;

    protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // MIPIDSITESTING_H
