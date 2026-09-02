/**
 * @file battery_lib.h
 * @brief Header file for BatteryTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef BATTERYTESTING_H
#define BATTERYTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class BatteryTesting : public TestBase
 * @brief Class for testing battery management functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class BatteryTesting : public TestBase {
public:
  BatteryTesting();
  ~BatteryTesting();

  explicit BatteryTesting(std::shared_ptr<SysfsProvider> sysfs,
                          std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate /sys/class/power_supply entries.
  TestOutput testPowerSupplyEnumeration();
  /// Check that at least one battery supply is present.
  TestOutput testBatteryPresence();
  /// Read and range-check capacity (0-100 %).
  TestOutput testCapacityRead();
  /// Read voltage_now/current_now.
  TestOutput testVoltageCurrent();
  /// Compute health from (energy|charge)_full vs *_design.
  TestOutput testBatteryHealth();
  /// Read and validate the supply status string.
  TestOutput testStatusRead();

  std::vector<std::string> getAvailableTests() override;
  std::vector<std::string> getPeriodicSafeTests();
  std::vector<std::string> getOneShotOnlyTests();

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;
  TestResult runSingleTest(const std::string &testName) override;

  /// First battery-class power supply name ("" if none).
  std::string firstBatterySupply() const;

protected:
  std::vector<TestResult> runAllTests() override;
};

#endif // BATTERYTESTING_H
