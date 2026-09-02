/**
 * @file spi_lib.h
 * @brief Header file for SpiTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef SPITESTING_H
#define SPITESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class SpiTesting : public TestBase
 * @brief Class for testing SPI bus functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class SpiTesting : public TestBase {
public:
  SpiTesting();
  ~SpiTesting();

  explicit SpiTesting(std::shared_ptr<SysfsProvider> sysfs,
                      std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate /dev/spidev* character devices.
  TestOutput testSpidevEnumeration();
  /// List /sys/bus/spi/devices entries.
  TestOutput testSysfsDevices();
  /// Read the modalias of the first SPI device.
  TestOutput testModaliasRead();
  /// Run spidev_test loopback transfer (if tool + device available).
  TestOutput testLoopbackTransfer();
  /// Check driver binding for SPI devices.
  TestOutput testDriverBinding();

  std::vector<std::string> getAvailableTests() override;
  std::vector<std::string> getPeriodicSafeTests();
  std::vector<std::string> getOneShotOnlyTests();

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;
  TestResult runSingleTest(const std::string &testName) override;

protected:
  std::vector<TestResult> runAllTests() override;
};

#endif // SPITESTING_H
