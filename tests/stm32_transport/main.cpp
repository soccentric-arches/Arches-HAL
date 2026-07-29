#include "stm32_transport.h"

#include <cstdlib>
#include <iostream>

namespace {

bool expect(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << "\n";
    return false;
  }
  return true;
}

} // namespace

int main() {
  setenv("ARCHES_STM32_MOCK", "1", 1);
  setenv("ARCHES_STM32_TRANSPORT", "uart", 1);

  Stm32Transport transport;
  const Stm32ProbeResult probe = transport.probe();
  if (!expect(probe.transportReady, "mock transport should report ready"))
    return 1;

  const Stm32RoundTripResult ping =
      transport.roundTrip({0x01, {'H', 'A', 'L', '?'}, 0});
  if (!expect(ping.status == hal::Status::PASS,
              "ping should succeed in mock mode"))
    return 1;
  if (!expect(!ping.response.empty(), "ping should return a payload"))
    return 1;

  setenv("ARCHES_STM32_FORCE_TIMEOUT", "1", 1);
  const Stm32RoundTripResult timeout = transport.roundTrip({0x01, {'T'}, 0});
  unsetenv("ARCHES_STM32_FORCE_TIMEOUT");
  if (!expect(timeout.status == hal::Status::TIMEOUT,
              "forced timeout should surface as TIMEOUT"))
    return 1;

  return 0;
}