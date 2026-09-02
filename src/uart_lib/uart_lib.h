/**
 * @file uart_lib.h
 * @brief Header file for UartTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef UARTTESTING_H
#define UARTTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class UartTesting : public TestBase
 * @brief Class for testing UART/serial functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class UartTesting : public TestBase {
public:
  UartTesting();
  ~UartTesting();

  explicit UartTesting(std::shared_ptr<SysfsProvider> sysfs,
                       std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate real serial ports (tty entries backed by a device).
  TestOutput testSerialPortEnumeration();
  /// Parse console= parameters from /proc/cmdline.
  TestOutput testConsoleDetection();
  /// Try to open the first serial device node.
  TestOutput testSerialPortOpen();
  /// Query the configured baud rate with stty.
  TestOutput testBaudRateRead();
  /// Resolve the bound kernel driver for the first serial port.
  TestOutput testDriverBinding();

  std::vector<std::string> getAvailableTests() override;
  std::vector<std::string> getPeriodicSafeTests();
  std::vector<std::string> getOneShotOnlyTests();

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;
  TestResult runSingleTest(const std::string &testName) override;

  /// First real serial device name in /sys/class/tty ("" if none).
  std::string firstSerialPort() const;

protected:
  std::vector<TestResult> runAllTests() override;
};

#endif // UARTTESTING_H
