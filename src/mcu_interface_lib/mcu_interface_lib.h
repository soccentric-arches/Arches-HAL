#ifndef MCUINTERFACETESTING_H
#define MCUINTERFACETESTING_H

#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>

class Stm32Transport;

class McuInterfaceTesting : public TestBase {
public:
  McuInterfaceTesting();
  ~McuInterfaceTesting() override;

  TestOutput testMcuCommunicationInterface();
  TestOutput testMcuFirmwareOperation();
  TestOutput testMcuWatchdog();
  TestOutput testMcuGpioInterface();
  TestOutput testMcuSpiInterface();
  TestOutput testMcuUartInterface();
  TestOutput testMcuDmsInterface();
  TestOutput testMcuToOrinDataTransfer();
  TestOutput testSecondaryMcuRedundancy();
  TestOutput testMcuFailoverMechanism();
  TestOutput testMcuSynchronization();

  std::vector<std::string> getAvailableTests() override;
  /// @brief Constructor with injected providers for deterministic testing
  explicit McuInterfaceTesting(std::shared_ptr<SysfsProvider> sysfs,
                               std::shared_ptr<CommandProvider> cmd = nullptr);

  /// @brief Get periodic-safe test names (passive health checks only)
  std::vector<std::string> getPeriodicSafeTests() const;

  /// @brief Get one-shot only test names (destructive/state-changing)
  std::vector<std::string> getOneShotOnlyTests() const;

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;

  TestResult runSingleTest(const std::string &testName) override;

protected:
  std::vector<TestResult> runAllTests() override;

private:
  TestOutput runPingTest(const std::string &name, std::uint8_t command,
                         const std::vector<std::uint8_t> &payload);
  std::vector<std::string> contractLines() const;

  std::unique_ptr<Stm32Transport> transport_;
  std::string transportSummary_;
};

#endif // MCUINTERFACETESTING_H
