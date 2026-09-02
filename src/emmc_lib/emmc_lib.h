/**
 * @file emmc_lib.h
 * @brief Header file for EmmcTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef EMMCTESTING_H
#define EMMCTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class EmmcTesting : public TestBase
 * @brief Class for testing eMMC storage functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class EmmcTesting : public TestBase {
public:
  EmmcTesting();
  ~EmmcTesting();

  explicit EmmcTesting(std::shared_ptr<SysfsProvider> sysfs,
                       std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate MMC hosts in /sys/class/mmc_host.
  TestOutput testMmcHostEnumeration();
  /// Detect an eMMC device (type "MMC") under any host.
  TestOutput testEmmcDevicePresence();
  /// Read the device name/model string.
  TestOutput testDeviceNameRead();
  /// Parse life_time health estimates (0x01-0x0B).
  TestOutput testLifeTimeHealth();
  /// Parse pre_eol_info (0x01 normal, 0x02 warning, 0x03 urgent).
  TestOutput testPreEolInfo();
  /// Read block device size in 512-byte sectors.
  TestOutput testBlockDeviceSize();

  std::vector<std::string> getAvailableTests() override;
  std::vector<std::string> getPeriodicSafeTests();
  std::vector<std::string> getOneShotOnlyTests();

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;
  TestResult runSingleTest(const std::string &testName) override;

  /// Sysfs directory of the first eMMC card ("" if none), e.g.
  /// /sys/class/mmc_host/mmc0/mmc0:0001.
  std::string firstEmmcCardDir() const;

protected:
  std::vector<TestResult> runAllTests() override;
};

#endif // EMMCTESTING_H
