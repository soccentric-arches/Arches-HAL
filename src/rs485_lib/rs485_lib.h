/**
 * @file rs485_lib.h
 * @brief Header file for Rs485Testing class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef RS485TESTING_H
#define RS485TESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class Rs485Testing : public TestBase
 * @brief Class for testing RS-485 serial bus functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class Rs485Testing : public TestBase {
public:
  Rs485Testing();
  ~Rs485Testing();

  explicit Rs485Testing(std::shared_ptr<SysfsProvider> sysfs,
                        std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate serial tty devices.
  TestOutput testSerialDeviceEnumeration();
  /// Query RS-485 mode support with TIOCGRS485.
  TestOutput testRs485Query();
  /// Report the RTS-on-send flag state.
  TestOutput testRtsOnSendFlag();
  /// Report the RX-during-TX flag state.
  TestOutput testRxDuringTxFlag();
  /// Resolve the bound kernel driver for the first serial port.
  TestOutput testDriverBinding();

  std::vector<std::string> getAvailableTests() override;
  std::vector<std::string> getPeriodicSafeTests();
  std::vector<std::string> getOneShotOnlyTests();

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;
  TestResult runSingleTest(const std::string &testName) override;

  /// Open the first serial device; returns fd or -1. Fills path.
  int openFirstSerialPort(std::string &path) const;

protected:
  std::vector<TestResult> runAllTests() override;
};

#endif // RS485TESTING_H
