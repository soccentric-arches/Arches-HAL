/**
 * @file nvme_lib.h
 * @brief Header file for NvmeTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef NVMETESTING_H
#define NVMETESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class NvmeTesting : public TestBase
 * @brief Class for testing NVMe storage functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class NvmeTesting : public TestBase {
    public:
    NvmeTesting();
    ~NvmeTesting();

    explicit NvmeTesting(std::shared_ptr<SysfsProvider> sysfs,
                         std::shared_ptr<CommandProvider> cmd = nullptr);

    /// Enumerate NVMe controllers in /sys/class/nvme.
    TestOutput testControllerEnumeration();
    /// Enumerate namespaces of the first controller.
    TestOutput testNamespaceEnumeration();
    /// Read controller state (expect "live").
    TestOutput testControllerState();
    /// Query SMART health log via nvme-cli (if installed).
    TestOutput testSmartHealth();
    /// Count NVMe block devices from /proc/partitions.
    TestOutput testPartitionEnumeration();

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

#endif // NVMETESTING_H
