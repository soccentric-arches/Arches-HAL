/**
 * @file i2c_lib.h
 * @brief Header file for I2cTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef I2CTESTING_H
#define I2CTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class I2cTesting : public TestBase
 * @brief Class for testing I2C bus functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class I2cTesting : public TestBase {
public:
  I2cTesting();
  ~I2cTesting();

  explicit I2cTesting(std::shared_ptr<SysfsProvider> sysfs,
                      std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate /dev/i2c-* adapter nodes.
  TestOutput testBusEnumeration();
  /// Run i2cdetect on the first bus to probe for devices.
  TestOutput testI2cDetectScan();
  /// List /sys/bus/i2c/devices entries.
  TestOutput testSysfsDevices();
  /// Read the adapter name of the first bus.
  TestOutput testAdapterNameRead();
  /// Check driver binding for i2c client devices.
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

#endif // I2CTESTING_H
