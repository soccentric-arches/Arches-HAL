/**
 * @file sd_card_lib.h
 * @brief Header file for SdCardTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef SDCARDTESTING_H
#define SDCARDTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class SdCardTesting : public TestBase
 * @brief Class for testing SD card functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class SdCardTesting : public TestBase {
    public:
    SdCardTesting();
    ~SdCardTesting();

    explicit SdCardTesting(std::shared_ptr<SysfsProvider> sysfs,
                           std::shared_ptr<CommandProvider> cmd = nullptr);

    /// Enumerate MMC hosts in /sys/class/mmc_host.
    TestOutput testMmcHostEnumeration();
    /// Detect an SD-type card under any host.
    TestOutput testSdCardPresence();
    /// Read the card CID (product serial identification).
    TestOutput testCidRead();
    /// Read block device size in 512-byte sectors.
    TestOutput testBlockDeviceSize();
    /// Read write-protect / read-only state.
    TestOutput testWriteProtectState();

    std::vector<std::string> getAvailableTests() override;
    std::vector<std::string> getPeriodicSafeTests();
    std::vector<std::string> getOneShotOnlyTests();

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;
    TestResult runSingleTest(const std::string& testName) override;

    /// Sysfs directory of the first SD-type card ("" if none).
    std::string firstSdCardDir() const;

    protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // SDCARDTESTING_H
