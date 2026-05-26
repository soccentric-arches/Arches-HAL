#include "stm32_transport.h"

#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {

bool expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        return false;
    }
    return true;
}

} // namespace

int main()
{
    const char* pinmuxFile = "/tmp/arches-pinmux-test.txt";
    {
        std::ofstream out(pinmuxFile);
        out << "uart spi gpio";
    }

    setenv("ARCHES_STM32_MOCK", "0", 1);
    setenv("ARCHES_STM32_TRANSPORT", "spi", 1);
    setenv("ARCHES_STM32_DEVICE", "/dev/null", 1);
    setenv("ARCHES_TEGRA_PINMUX_PATH", pinmuxFile, 1);

    Stm32Transport transport;
    const Stm32ProbeResult probe = transport.probe();

    if (!expect(probe.pinmuxConfigured, "probe should validate the supplied pinmux content")) return 1;
    if (!expect(probe.hostDevicePresent, "probe should accept an existing transport node")) return 1;

    return 0;
}