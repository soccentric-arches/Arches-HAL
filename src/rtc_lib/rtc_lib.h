/**
 * @file rtc_lib.h
 * @brief Header file for RtcTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef RTCTESTING_H
#define RTCTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class RtcTesting : public TestBase
 * @brief Class for testing real-time clock functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class RtcTesting : public TestBase {
public:
  RtcTesting();
  ~RtcTesting();

  explicit RtcTesting(std::shared_ptr<SysfsProvider> sysfs,
                      std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Check that an rtc class device is present.
  TestOutput testRtcDevicePresence();
  /// Read and validate RTC time and date.
  TestOutput testRtcTimeRead();
  /// Check whether the kernel synced the RTC to the system clock (hctosys).
  TestOutput testHctosys();
  /// Check alarm/wakealarm capability.
  TestOutput testAlarmCapability();
  /// Compare RTC clock to the system clock and report the skew.
  TestOutput testClockSkew();

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

#endif // RTCTESTING_H
