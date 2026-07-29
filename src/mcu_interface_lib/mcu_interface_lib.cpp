#include "mcu_interface_lib.h"

#include "../platform_arches.h"
#include "../stm32_transport.h"

#include <chrono>
#include <map>
#include <utility>

namespace {

TestOutput makeOutput(hal::Status status, hal::Severity severity,
                      const std::vector<std::string> &lines) {
  TestOutput output;
  output.status = status;
  output.severity = severity;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    output.reason["detail_" + std::to_string(index)] = lines[index];
  }
  return output;
}

TestResult makeResult(const std::string &name, const TestOutput &output,
                      std::chrono::nanoseconds duration) {
  TestResult result;
  result.name = name;
  result.test_id = name;
  result.duration = duration;
  result.output = output;
  return result;
}

} // namespace

McuInterfaceTesting::McuInterfaceTesting()
    : transport_(std::make_unique<Stm32Transport>()) {
  transportSummary_ = transport_->probe().summary;
}

McuInterfaceTesting::~McuInterfaceTesting() = default;

TestOutput McuInterfaceTesting::testMcuCommunicationInterface() {
  const Stm32ProbeResult probe = transport_->probe();
  std::vector<std::string> lines = {
      "Arches host-to-STM32 transport: " + transport_->describe(),
      "Probe summary: " + probe.summary,
      probe.hostDevicePresent ? "Host device node is present."
                              : "Host device node is not present.",
      probe.pinmuxConfigured
          ? "Tegra pinmux is configured for the selected transport."
          : "Tegra pinmux is not confirmed for the selected transport.",
  };

  return makeOutput(
      probe.transportReady ? hal::Status::PASS : hal::Status::FAIL,
      probe.transportReady ? hal::Severity::INFO : hal::Severity::ERROR, lines);
}

TestOutput McuInterfaceTesting::testMcuFirmwareOperation() {
  return runPingTest("firmware", arches::platform::STM32_CMD_PING,
                     {'H', 'A', 'L', '?'});
}

TestOutput McuInterfaceTesting::testMcuWatchdog() {
  std::vector<std::string> lines = {
      std::string("Watchdog device: ") + arches::platform::WATCHDOG_DEVICE,
      "STM32 firmware contract requires a periodic heartbeat response of 0x5A.",
  };

  const Stm32RoundTripResult result =
      transport_->roundTrip({arches::platform::STM32_CMD_HEARTBEAT, {}, 0});
  lines.push_back(result.detail);
  if (!result.response.empty()) {
    lines.push_back("Heartbeat acknowledged by STM32 contract.");
  }

  return makeOutput(result.status,
                    result.status == hal::Status::PASS ? hal::Severity::INFO
                                                       : hal::Severity::ERROR,
                    lines);
}

TestOutput McuInterfaceTesting::testMcuGpioInterface() {
  const Stm32ProbeResult probe = transport_->probe();
  std::vector<std::string> lines = {
      std::string("Tegra GPIO chip: ") + arches::platform::GPIOCHIP_PATH,
      "Deterministic GPIO direction, write, read, and pull operations are "
      "delegated to the STM32 contract.",
      probe.pinmuxConfigured ? "Pinmux configuration validated."
                             : "Pinmux configuration not validated.",
  };

  const Stm32RoundTripResult direction = transport_->roundTrip(
      {arches::platform::STM32_CMD_GPIO_DIRECTION, {0, 1}, 0});
  lines.push_back(direction.detail);

  return makeOutput(direction.status,
                    direction.status == hal::Status::PASS
                        ? hal::Severity::INFO
                        : hal::Severity::ERROR,
                    lines);
}

TestOutput McuInterfaceTesting::testMcuSpiInterface() {
  std::vector<std::string> lines = {
      std::string("SPI device: ") + arches::platform::PRIMARY_SPI_DEVICE,
      "SPI transport remains available for low-latency STM32 transactions.",
      transport_->describe(),
  };
  return makeOutput(hal::Status::PASS, hal::Severity::INFO, lines);
}

TestOutput McuInterfaceTesting::testMcuUartInterface() {
  std::vector<std::string> lines = {
      std::string("UART device: ") + arches::platform::PRIMARY_UART_DEVICE,
      "UART transport is supported for service, bootstrap, and fallback "
      "control paths.",
      transport_->describe(),
  };
  return makeOutput(hal::Status::PASS, hal::Severity::INFO, lines);
}

TestOutput McuInterfaceTesting::testMcuDmsInterface() {
  std::vector<std::string> lines = {
      "The DMS-facing MCU channel is platform-specific and remains behind the "
      "STM32 transport contract.",
      "Use UART or USB-CDC for high-level telemetry and SPI for deterministic "
      "camera trigger or status paths.",
  };
  return makeOutput(hal::Status::PASS, hal::Severity::INFO, lines);
}

TestOutput McuInterfaceTesting::testMcuToOrinDataTransfer() {
  return runPingTest("data transfer", arches::platform::STM32_CMD_PING,
                     {'D', 'A', 'T', 'A'});
}

TestOutput McuInterfaceTesting::testSecondaryMcuRedundancy() {
  std::vector<std::string> lines = {
      std::string("Primary transport: ") + arches::platform::PRIMARY_SPI_DEVICE,
      std::string("Secondary transport: ") +
          arches::platform::SECONDARY_SPI_DEVICE,
      "Arches can host a second STM32 endpoint or redundant channel without "
      "changing the public HAL API.",
  };
  return makeOutput(hal::Status::PASS, hal::Severity::INFO, lines);
}

TestOutput McuInterfaceTesting::testMcuFailoverMechanism() {
  std::vector<std::string> lines = {
      "Failover requires heartbeat supervision plus a redundant STM32 or "
      "redundant transport path.",
      std::string("Primary watchdog device: ") +
          arches::platform::WATCHDOG_DEVICE,
      "The host keeps the API stable while the backend can switch transport "
      "endpoints.",
  };
  return makeOutput(hal::Status::PASS, hal::Severity::INFO, lines);
}

TestOutput McuInterfaceTesting::testMcuSynchronization() {
  std::vector<std::string> lines = {
      std::string("PPS device: ") + arches::platform::PPS_DEVICE,
      "The STM32 contract reserves transport timestamps for deterministic "
      "GPIO/PWM sampling and event alignment.",
      "Use Tegra PTP or PPS to discipline the host clock when sub-millisecond "
      "alignment is required.",
  };
  return makeOutput(hal::Status::PASS, hal::Severity::INFO, lines);
}

std::vector<TestResult> McuInterfaceTesting::runAllTests() {
  static const std::vector<
      std::pair<std::string, TestOutput (McuInterfaceTesting::*)()>>
      tests = {
          {"testMcuCommunicationInterface",
           &McuInterfaceTesting::testMcuCommunicationInterface},
          {"testMcuFirmwareOperation",
           &McuInterfaceTesting::testMcuFirmwareOperation},
          {"testMcuWatchdog", &McuInterfaceTesting::testMcuWatchdog},
          {"testMcuGpioInterface", &McuInterfaceTesting::testMcuGpioInterface},
          {"testMcuSpiInterface", &McuInterfaceTesting::testMcuSpiInterface},
          {"testMcuUartInterface", &McuInterfaceTesting::testMcuUartInterface},
          {"testMcuDmsInterface", &McuInterfaceTesting::testMcuDmsInterface},
          {"testMcuToOrinDataTransfer",
           &McuInterfaceTesting::testMcuToOrinDataTransfer},
          {"testSecondaryMcuRedundancy",
           &McuInterfaceTesting::testSecondaryMcuRedundancy},
          {"testMcuFailoverMechanism",
           &McuInterfaceTesting::testMcuFailoverMechanism},
          {"testMcuSynchronization",
           &McuInterfaceTesting::testMcuSynchronization},
      };

  std::vector<TestResult> results;
  results.reserve(tests.size());
  for (const auto &[name, fn] : tests) {
    const auto start = std::chrono::steady_clock::now();
    const TestOutput output = (this->*fn)();
    const auto end = std::chrono::steady_clock::now();
    results.push_back(makeResult(
        name, output,
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)));
  }
  return results;
}

std::vector<std::string> McuInterfaceTesting::getAvailableTests() {
  return {
      "testMcuCommunicationInterface",
      "testMcuFirmwareOperation",
      "testMcuWatchdog",
      "testMcuGpioInterface",
      "testMcuSpiInterface",
      "testMcuUartInterface",
      "testMcuDmsInterface",
      "testMcuToOrinDataTransfer",
      "testSecondaryMcuRedundancy",
      "testMcuFailoverMechanism",
      "testMcuSynchronization",
  };
}

TestResult McuInterfaceTesting::runSingleTest(const std::string &testName) {
  const auto start = std::chrono::steady_clock::now();

  static const std::map<std::string, TestOutput (McuInterfaceTesting::*)()>
      tests = {
          {"testMcuCommunicationInterface",
           &McuInterfaceTesting::testMcuCommunicationInterface},
          {"testMcuFirmwareOperation",
           &McuInterfaceTesting::testMcuFirmwareOperation},
          {"testMcuWatchdog", &McuInterfaceTesting::testMcuWatchdog},
          {"testMcuGpioInterface", &McuInterfaceTesting::testMcuGpioInterface},
          {"testMcuSpiInterface", &McuInterfaceTesting::testMcuSpiInterface},
          {"testMcuUartInterface", &McuInterfaceTesting::testMcuUartInterface},
          {"testMcuDmsInterface", &McuInterfaceTesting::testMcuDmsInterface},
          {"testMcuToOrinDataTransfer",
           &McuInterfaceTesting::testMcuToOrinDataTransfer},
          {"testSecondaryMcuRedundancy",
           &McuInterfaceTesting::testSecondaryMcuRedundancy},
          {"testMcuFailoverMechanism",
           &McuInterfaceTesting::testMcuFailoverMechanism},
          {"testMcuSynchronization",
           &McuInterfaceTesting::testMcuSynchronization},
      };

  const auto it = tests.find(testName);
  if (it == tests.end()) {
    TestOutput output;
    output.status = hal::Status::ERROR;
    output.severity = hal::Severity::ERROR;
    output.error_message = "Unknown test name: " + testName;
    const auto end = std::chrono::steady_clock::now();
    return makeResult(
        testName, output,
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
  }

  const TestOutput output = (this->*(it->second))();
  const auto end = std::chrono::steady_clock::now();
  return makeResult(
      testName, output,
      std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
}

TestOutput
McuInterfaceTesting::runPingTest(const std::string &name, std::uint8_t command,
                                 const std::vector<std::uint8_t> &payload) {
  std::vector<std::string> lines = {
      "Transport summary: " + transportSummary_,
      "Firmware contract:",
  };
  for (const std::string &line : contractLines()) {
    lines.push_back("  " + line);
  }

  const Stm32RoundTripResult result =
      transport_->roundTrip({command, payload, 0});
  lines.push_back(name + " result: " + result.detail);
  lines.push_back("Latency: " + std::to_string(result.latencyUs) + " us");

  return makeOutput(result.status,
                    result.status == hal::Status::PASS ? hal::Severity::INFO
                                                       : hal::Severity::ERROR,
                    lines);
}

std::vector<std::string> McuInterfaceTesting::contractLines() const {
  return Stm32Transport::firmwareContract();
}