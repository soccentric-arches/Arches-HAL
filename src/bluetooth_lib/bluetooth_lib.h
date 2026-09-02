/**
 * @file bluetooth_lib.h
 * @brief Header file for BluetoothTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef BLUETOOTHTESTING_H
#define BLUETOOTHTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class BluetoothTesting : public TestBase
 * @brief Class for testing Bluetooth radio functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class BluetoothTesting : public TestBase {
public:
  BluetoothTesting();
  ~BluetoothTesting();

  explicit BluetoothTesting(std::shared_ptr<SysfsProvider> sysfs,
                            std::shared_ptr<CommandProvider> cmd = nullptr);

  /// Enumerate hci* adapters in /sys/class/bluetooth.
  TestOutput testAdapterEnumeration();
  /// Read the adapter MAC address.
  TestOutput testAdapterAddress();
  /// Inspect rfkill state for bluetooth devices.
  TestOutput testRfkillState();
  /// Check adapter power state via hciconfig (if installed).
  TestOutput testPoweredState();
  /// Check the BlueZ daemon status (if systemctl available).
  TestOutput testBluezDaemon();

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

#endif // BLUETOOTHTESTING_H
