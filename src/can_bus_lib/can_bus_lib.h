/**
 * @file can_bus_lib.h
 * @brief Header file for CanBusTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef CANBUSTESTING_H
#define CANBUSTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class CanBusTesting : public TestBase
 * @brief Class for testing CAN bus functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class CanBusTesting : public TestBase {
    public:
    CanBusTesting();
    ~CanBusTesting();

    explicit CanBusTesting(std::shared_ptr<SysfsProvider> sysfs,
                           std::shared_ptr<CommandProvider> cmd = nullptr);

    /// Enumerate can* network interfaces.
    TestOutput testInterfaceEnumeration();
    /// Read the operational state of the first CAN interface.
    TestOutput testOperState();
    /// Read interface statistics (packets and error counters).
    TestOutput testStatistics();
    /// Parse the configured bitrate via `ip -details`.
    TestOutput testBitrateRead();
    /// Read the CAN bus state (ERROR-ACTIVE etc.).
    TestOutput testCanState();

    std::vector<std::string> getAvailableTests() override;
    std::vector<std::string> getPeriodicSafeTests();
    std::vector<std::string> getOneShotOnlyTests();

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;
    TestResult runSingleTest(const std::string& testName) override;

    /// First can* interface name ("" if none).
    std::string firstCanInterface() const;

    protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // CANBUSTESTING_H
