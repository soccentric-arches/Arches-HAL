#pragma once

#include "platform_arches.h"

// Compatibility aliases retained while the remaining modules are migrated to
// the shared Arches platform header. Public HAL APIs stay unchanged.
static constexpr int RPI5_CPU_CORES = arches::platform::CPU_CORES_MAX;
static constexpr double RPI5_CPU_MAX_FREQ_MHZ = 2200.0;
static constexpr double RPI5_CPU_MIN_FREQ_MHZ = 729.0;
static constexpr double RPI5_CPU_TEMP_MAX_C = arches::platform::CPU_TEMP_MAX_C;

static constexpr int RPI5_GPU_SHADER_CORES = 2048;
static constexpr double RPI5_GPU_MAX_FREQ_MHZ = 1300.0;
static constexpr double RPI5_GPU_MIN_FREQ_MHZ = 300.0;
static constexpr double RPI5_GPU_TEMP_MAX_C = arches::platform::GPU_TEMP_MAX_C;
static constexpr const char* RPI5_GPU_DEVFREQ_PATH = "/sys/class/devfreq/17000000.ga10b";
static constexpr const char* RPI5_GPU_DEVICE_PATH = "/sys/devices/platform/17000000.ga10b";
static constexpr const char* RPI5_V3D_DEV_PATH = "/dev/dri/card0";
static constexpr const char* RPI5_V3D_RENDER_PATH = arches::platform::GPU_RENDER_NODE;

static constexpr long long RPI5_MEM_8GB_BYTES = 8LL * 1024 * 1024 * 1024;
static constexpr long long RPI5_MEM_4GB_BYTES = 4LL * 1024 * 1024 * 1024;
static constexpr double RPI5_MEM_SPEED_MHZ = 3200.0;
static constexpr double RPI5_MEM_BW_GBS = 102.4;
static constexpr int RPI5_MEM_BUS_BITS = 256;

static constexpr int RPI5_PCIE_GEN = 4;
static constexpr int RPI5_PCIE_LANES = 4;
static constexpr double RPI5_PCIE_BW_GBS = 7.88;
static constexpr const char* RPI5_PCIE_RC_PATH = "/sys/bus/pci/devices/0000:00:00.0";

static constexpr const char* RPI5_XHCI_DRIVER_PATH = "/sys/bus/platform/drivers/tegra-xusb";
static constexpr int RPI5_USB3_PORT_COUNT = 2;
static constexpr int RPI5_USB2_PORT_COUNT = 2;

static constexpr const char* RPI5_ETH_IFACE = arches::platform::ETHERNET_IFACE;
static constexpr int RPI5_ETH_SPEED_MBPS = 1000;

static constexpr const char* RPI5_CSI_DEVICE0 = "/dev/video0";
static constexpr const char* RPI5_CSI_DEVICE1 = "/dev/video1";
static constexpr const char* RPI5_MEDIA_CTL0 = "/dev/media0";
static constexpr const char* RPI5_UNICAM_PATH = "/sys/bus/platform/devices/15a00000.nvcsi";

static constexpr const char* RPI5_DRM_CARD0 = "/dev/dri/card0";
static constexpr const char* RPI5_DRM_CARD1 = "/dev/dri/card1";

static constexpr const char* RPI5_I2S_DEVICE = "/sys/bus/platform/devices/2901000.i2s";
static constexpr const char* RPI5_ALSA_CARDS = "/proc/asound/cards";

static constexpr const char* RPI5_DEBUG_UART = "/dev/ttyTCU0";
static constexpr const char* RPI5_UART1 = arches::platform::PRIMARY_UART_DEVICE;

static constexpr const char* RPI5_THERMAL_CPU_ZONE = "/sys/class/thermal/thermal_zone0";
static constexpr const char* RPI5_THERMAL_SB_ZONE = "/sys/class/thermal/thermal_zone1";
static constexpr double RPI5_THERMAL_THROTTLE_C = 90.0;
static constexpr double RPI5_THERMAL_SHUTDOWN_C = 95.0;

static constexpr const char* RPI5_GPIOCHIP_PATH = arches::platform::GPIOCHIP_PATH;
static constexpr int RPI5_GPIO_PIN_COUNT = 40;

static constexpr const char* RPI5_MMCBLK_PATH = "/dev/mmcblk0";
static constexpr const char* RPI5_NVME_PATH = "/dev/nvme0n1";

static constexpr const char* RPI5_POWER_SUPPLY_PATH = "/sys/class/power_supply";

static constexpr const char* RPI5_HAILO8_PCIE_VENDOR = "1e60";
static constexpr const char* RPI5_CORAL_USB_VENDOR = "18d1";

static constexpr const char* PLATFORM_NAME = arches::platform::PLATFORM_NAME;
static constexpr const char* PLATFORM_SOC = arches::platform::PLATFORM_SOC_FAMILY;
static constexpr const char* PLATFORM_IOBRIDGE = "STM32";
static constexpr const char* RPI5_DT_MODEL_PATH = "/proc/device-tree/model";
