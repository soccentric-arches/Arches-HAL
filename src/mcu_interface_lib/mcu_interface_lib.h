#ifndef MCUINTERFACETESTING_H
#define MCUINTERFACETESTING_H

#include "lib_base.h"

#include <memory>
#include <string>

class Stm32Transport;

class McuInterfaceTesting : public TestBase
{
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
    TestResult runSingleTest(const std::string& testName) override;

  protected:
    std::vector<TestResult> runAllTests() override;

  private:
    TestOutput runPingTest(const std::string& name, std::uint8_t command, const std::vector<std::uint8_t>& payload);
    std::vector<std::string> contractLines() const;

    std::unique_ptr<Stm32Transport> transport_;
    std::string transportSummary_;
};

#endif // MCUINTERFACETESTING_H
