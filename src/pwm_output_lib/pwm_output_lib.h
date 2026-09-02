/**
 * @file pwm_output_lib.h
 * @brief Header file for PwmOutputTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef PWMOUTPUTTESTING_H
#define PWMOUTPUTTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class PwmOutputTesting : public TestBase
 * @brief Class for testing PWM output functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 * Export/config/enable tests are state-changing and classified one-shot only.
 */
class PwmOutputTesting : public TestBase {
public:
  PwmOutputTesting();
  ~PwmOutputTesting();

  explicit PwmOutputTesting(std::shared_ptr<SysfsProvider> sysfs,
                            std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate pwmchip* entries in /sys/class/pwm.
  TestOutput testPwmChipEnumeration();
  /// Read npwm (channel count) of the first chip.
  TestOutput testChannelCount();
  /// Export then unexport channel 0 (one-shot).
  TestOutput testExportUnexport();
  /// Configure period/duty cycle and verify readback (one-shot).
  TestOutput testPeriodDutyConfig();
  /// Enable then disable the channel (one-shot).
  TestOutput testEnableDisable();

  std::vector<std::string> getAvailableTests() override;
  std::vector<std::string> getPeriodicSafeTests();
  std::vector<std::string> getOneShotOnlyTests();

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;
  TestResult runSingleTest(const std::string &testName) override;

  /// Sysfs dir of the first PWM chip ("" if none).
  std::string firstChipDir() const;
  /// Export channel 0 of the first chip; returns chip dir + pwm node or "".
  std::string exportChannel0(const std::string &chipDir) const;
  /// Unexport channel 0 of the first chip.
  void unexportChannel0(const std::string &chipDir) const;

protected:
  std::vector<TestResult> runAllTests() override;
};

#endif // PWMOUTPUTTESTING_H
