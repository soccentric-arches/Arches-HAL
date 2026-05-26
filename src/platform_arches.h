#pragma once

#include <array>
#include <cstdint>

namespace arches::platform {

inline constexpr const char* PLATFORM_NAME = "Arches";
inline constexpr const char* PLATFORM_SOC_FAMILY = "NVIDIA Tegra";
inline constexpr const char* PLATFORM_OS = "L4T / JetPack";

inline constexpr int CPU_CORES_MIN = 8;
inline constexpr int CPU_CORES_MAX = 12;
inline constexpr double CPU_TEMP_MAX_C = 95.0;

inline constexpr double GPU_TEMP_MAX_C = 95.0;
inline constexpr const char* GPU_RENDER_NODE = "/dev/dri/renderD128";

inline constexpr const char* CPU_FREQ_PATH = "/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq";

inline constexpr std::array<const char*, 2> GPU_FREQ_PATHS = {
    "/sys/class/devfreq/17000000.ga10b/cur_freq",
    "/sys/class/devfreq/gpu.0/cur_freq",
};

inline constexpr std::array<const char*, 3> THERMAL_ZONE_PATHS = {
    "/sys/class/thermal/thermal_zone0/temp",
    "/sys/class/thermal/thermal_zone1/temp",
    "/sys/class/thermal/thermal_zone2/temp",
};

inline constexpr const char* GPIOCHIP_PATH = "/dev/gpiochip0";
inline constexpr const char* PRIMARY_I2C_DEVICE = "/dev/i2c-1";
inline constexpr const char* PRIMARY_SPI_DEVICE = "/dev/spidev1.0";
inline constexpr const char* SECONDARY_SPI_DEVICE = "/dev/spidev1.1";
inline constexpr const char* PRIMARY_UART_DEVICE = "/dev/ttyTHS1";
inline constexpr const char* SECONDARY_UART_DEVICE = "/dev/ttyTHS2";
inline constexpr const char* USB_CDC_DEVICE = "/dev/ttyACM0";
inline constexpr const char* WATCHDOG_DEVICE = "/dev/watchdog0";
inline constexpr const char* PPS_DEVICE = "/dev/pps0";
inline constexpr const char* ETHERNET_IFACE = "eth0";

inline constexpr std::array<const char*, 3> PINMUX_PATHS = {
    "/sys/kernel/debug/pinctrl/2430000.pinmux/pinmux-pins",
    "/sys/kernel/debug/pinctrl/tegra-pinctrl/pinmux-pins",
    "/sys/kernel/debug/pinctrl/70003000.pinctrl/pinmux-pins",
};

inline constexpr const char* STM32_PROTOCOL_NAME = "arches-stm32-rtio";
inline constexpr std::uint8_t STM32_PROTOCOL_VERSION = 1;
inline constexpr std::uint8_t STM32_SOF = 0xA5;
inline constexpr std::uint8_t STM32_CMD_PING = 0x01;
inline constexpr std::uint8_t STM32_CMD_GPIO_DIRECTION = 0x10;
inline constexpr std::uint8_t STM32_CMD_GPIO_WRITE = 0x11;
inline constexpr std::uint8_t STM32_CMD_GPIO_READ = 0x12;
inline constexpr std::uint8_t STM32_CMD_GPIO_PULL = 0x13;
inline constexpr std::uint8_t STM32_CMD_HEARTBEAT = 0x20;

inline constexpr int STM32_DEFAULT_BAUD = 115200;
inline constexpr int STM32_DEFAULT_SPI_HZ = 1000000;
inline constexpr int STM32_DEFAULT_TIMEOUT_MS = 100;

} // namespace arches::platform