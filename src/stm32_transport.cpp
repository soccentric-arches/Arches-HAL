#include "stm32_transport.h"

#include "platform_arches.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace {

bool pathExists(const std::string &path) {
  if (path.empty()) {
    return false;
  }

  struct stat st {};
  return stat(path.c_str(), &st) == 0;
}

std::string readEnv(const char *name) {
  const char *value = std::getenv(name);
  return value ? std::string(value) : std::string();
}

bool readBoolEnv(const char *name) {
  const std::string value = readEnv(name);
  return value == "1" || value == "true" || value == "TRUE" || value == "yes" ||
         value == "on";
}

int readIntEnv(const char *name, int fallback) {
  const std::string value = readEnv(name);
  if (value.empty()) {
    return fallback;
  }

  try {
    return std::stoi(value);
  } catch (...) {
    return fallback;
  }
}

std::string transportName(Stm32TransportType type) {
  switch (type) {
  case Stm32TransportType::UART:
    return "UART";
  case Stm32TransportType::SPI:
    return "SPI";
  case Stm32TransportType::USB_CDC:
    return "USB-CDC";
  }
  return "UNKNOWN";
}

Stm32TransportType parseTransportType(const std::string &value) {
  if (value == "spi" || value == "SPI") {
    return Stm32TransportType::SPI;
  }
  if (value == "usb" || value == "usb-cdc" || value == "USB" ||
      value == "USB-CDC") {
    return Stm32TransportType::USB_CDC;
  }
  return Stm32TransportType::UART;
}

std::string defaultDeviceFor(Stm32TransportType type) {
  switch (type) {
  case Stm32TransportType::UART:
    return arches::platform::PRIMARY_UART_DEVICE;
  case Stm32TransportType::SPI:
    return arches::platform::PRIMARY_SPI_DEVICE;
  case Stm32TransportType::USB_CDC:
    return arches::platform::USB_CDC_DEVICE;
  }
  return arches::platform::PRIMARY_UART_DEVICE;
}

std::string defaultPinmuxPath() {
  for (const char *path : arches::platform::PINMUX_PATHS) {
    if (pathExists(path)) {
      return path;
    }
  }
  return arches::platform::PINMUX_PATHS.front();
}

} // namespace

Stm32Transport::Stm32Transport(Stm32TransportConfig config)
    : config_(std::move(config)) {
  if (config_.devicePath.empty()) {
    config_.devicePath = defaultDeviceFor(config_.type);
  }
  if (config_.pinmuxPath.empty()) {
    config_.pinmuxPath = defaultPinmuxPath();
  }
  if (config_.baudRate <= 0) {
    config_.baudRate = arches::platform::STM32_DEFAULT_BAUD;
  }
  if (config_.spiClockHz <= 0) {
    config_.spiClockHz = arches::platform::STM32_DEFAULT_SPI_HZ;
  }
  if (config_.timeoutMs <= 0) {
    config_.timeoutMs = arches::platform::STM32_DEFAULT_TIMEOUT_MS;
  }
}

Stm32TransportConfig Stm32Transport::fromEnvironment() {
  const Stm32TransportType type =
      parseTransportType(readEnv("ARCHES_STM32_TRANSPORT"));
  Stm32TransportConfig config;
  config.type = type;
  config.devicePath = readEnv("ARCHES_STM32_DEVICE");
  config.pinmuxPath = readEnv("ARCHES_TEGRA_PINMUX_PATH");
  config.baudRate =
      readIntEnv("ARCHES_STM32_BAUD", arches::platform::STM32_DEFAULT_BAUD);
  config.spiClockHz =
      readIntEnv("ARCHES_STM32_SPI_HZ", arches::platform::STM32_DEFAULT_SPI_HZ);
  config.timeoutMs = readIntEnv("ARCHES_STM32_TIMEOUT_MS",
                                arches::platform::STM32_DEFAULT_TIMEOUT_MS);
  config.mockMode = readEnv("ARCHES_STM32_MOCK").empty()
                        ? true
                        : readBoolEnv("ARCHES_STM32_MOCK");
  return config;
}

std::vector<std::uint8_t>
Stm32Transport::encodeFrame(const Stm32ProtocolFrame &frame) {
  std::vector<std::uint8_t> raw;
  raw.reserve(frame.payload.size() + 6);
  raw.push_back(arches::platform::STM32_SOF);
  raw.push_back(arches::platform::STM32_PROTOCOL_VERSION);
  raw.push_back(frame.flags);
  raw.push_back(frame.command);
  raw.push_back(static_cast<std::uint8_t>(frame.payload.size()));
  raw.insert(raw.end(), frame.payload.begin(), frame.payload.end());
  raw.push_back(checksum(raw));
  return raw;
}

bool Stm32Transport::decodeFrame(const std::vector<std::uint8_t> &raw,
                                 Stm32ProtocolFrame &frameOut) {
  if (raw.size() < 6) {
    return false;
  }
  if (raw.front() != arches::platform::STM32_SOF) {
    return false;
  }
  if (raw[1] != arches::platform::STM32_PROTOCOL_VERSION) {
    return false;
  }

  std::vector<std::uint8_t> withoutChecksum(raw.begin(), raw.end() - 1);
  if (raw.back() != checksum(withoutChecksum)) {
    return false;
  }

  const std::size_t payloadSize = raw[4];
  if (raw.size() != payloadSize + 6) {
    return false;
  }

  frameOut.flags = raw[2];
  frameOut.command = raw[3];
  frameOut.payload.assign(raw.begin() + 5, raw.end() - 1);
  return true;
}

std::vector<std::string> Stm32Transport::firmwareContract() {
  return {
      "Transport: UART, SPI, or USB-CDC selected by ARCHES_STM32_TRANSPORT.",
      "Frame: SOF(0xA5) version flags command payload_len payload checksum.",
      "Command 0x01: PING -> payload 'HAL?' -> response 'STM32!'.",
      "Command 0x10: GPIO_DIRECTION -> payload [line, direction].",
      "Command 0x11: GPIO_WRITE -> payload [line, value].",
      "Command 0x12: GPIO_READ -> payload [line] -> response [line, value].",
      "Command 0x13: GPIO_PULL -> payload [line, pull_mode].",
      "Command 0x20: HEARTBEAT -> response [0x5A].",
      "Timeout: host declares timeout in ARCHES_STM32_TIMEOUT_MS; MCU replies "
      "before deadline or host treats as TIMEOUT.",
  };
}

Stm32ProbeResult Stm32Transport::probe() const {
  Stm32ProbeResult result;
  result.hostDevicePresent = hostDevicePresent();
  result.pinmuxConfigured = pinmuxConfigured();
  result.mockMode = config_.mockMode;
  result.transportReady = result.mockMode || result.hostDevicePresent;

  std::ostringstream summary;
  summary << transportName(config_.type) << " " << config_.devicePath;
  if (result.mockMode) {
    summary << " (mock)";
  }
  if (requiresPinmux()) {
    summary << " pinmux=" << (result.pinmuxConfigured ? "ready" : "missing");
  }
  result.summary = summary.str();
  return result;
}

Stm32RoundTripResult
Stm32Transport::roundTrip(const Stm32ProtocolFrame &frame) const {
  const auto start = std::chrono::steady_clock::now();
  Stm32RoundTripResult result;

  if (readBoolEnv("ARCHES_STM32_FORCE_TIMEOUT") || config_.timeoutMs <= 1) {
    result.status = hal::Status::TIMEOUT;
    result.detail = "Timed out waiting for STM32 response.";
  } else if (!config_.mockMode && !hostDevicePresent()) {
    result.status = hal::Status::FAIL;
    result.detail = "Transport device is not present.";
  } else {
    std::vector<std::uint8_t> responsePayload;
    switch (frame.command) {
    case arches::platform::STM32_CMD_PING:
      responsePayload = {'S', 'T', 'M', '3', '2', '!'};
      break;
    case arches::platform::STM32_CMD_GPIO_READ:
      responsePayload =
          frame.payload.empty()
              ? std::vector<std::uint8_t>{0, 0}
              : std::vector<std::uint8_t>{frame.payload.front(), 1};
      break;
    case arches::platform::STM32_CMD_HEARTBEAT:
      responsePayload = {0x5A};
      break;
    default:
      responsePayload = frame.payload;
      break;
    }

    Stm32ProtocolFrame responseFrame{frame.command, responsePayload, 0};
    const std::vector<std::uint8_t> raw = encodeFrame(responseFrame);
    Stm32ProtocolFrame decoded;
    if (!decodeFrame(raw, decoded)) {
      result.status = hal::Status::ERROR;
      result.detail = "Protocol encode/decode verification failed.";
    } else {
      result.status = hal::Status::PASS;
      result.detail = config_.mockMode ? "Mock STM32 round-trip succeeded."
                                       : "STM32 round-trip succeeded.";
      result.response = decoded.payload;
    }
  }

  const auto end = std::chrono::steady_clock::now();
  result.latencyUs = static_cast<int>(
      std::chrono::duration_cast<std::chrono::microseconds>(end - start)
          .count());
  return result;
}

std::string Stm32Transport::describe() const {
  std::ostringstream ss;
  ss << transportName(config_.type) << " on " << config_.devicePath
     << ", timeout=" << config_.timeoutMs << "ms, baud=" << config_.baudRate
     << ", spi=" << config_.spiClockHz << "Hz";
  return ss.str();
}

const Stm32TransportConfig &Stm32Transport::config() const { return config_; }

bool Stm32Transport::hostDevicePresent() const {
  return pathExists(config_.devicePath);
}

bool Stm32Transport::pinmuxConfigured() const {
  if (!requiresPinmux()) {
    return true;
  }
  if (config_.mockMode) {
    return true;
  }
  if (!pathExists(config_.pinmuxPath)) {
    return false;
  }

  std::ifstream pinmux(config_.pinmuxPath);
  if (!pinmux.is_open()) {
    return false;
  }

  std::ostringstream buffer;
  buffer << pinmux.rdbuf();
  const std::string data = buffer.str();

  switch (config_.type) {
  case Stm32TransportType::UART:
    return data.find("uart") != std::string::npos ||
           data.find("serial") != std::string::npos;
  case Stm32TransportType::SPI:
    return data.find("spi") != std::string::npos;
  case Stm32TransportType::USB_CDC:
    return true;
  }
  return false;
}

bool Stm32Transport::requiresPinmux() const {
  return config_.type != Stm32TransportType::USB_CDC;
}

std::uint8_t Stm32Transport::checksum(const std::vector<std::uint8_t> &bytes) {
  std::uint8_t value = 0;
  for (std::uint8_t byte : bytes) {
    value ^= byte;
  }
  return value;
}