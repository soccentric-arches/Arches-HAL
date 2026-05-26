#pragma once

#include "lib_base.h"

#include <cstdint>
#include <string>
#include <vector>

enum class Stm32TransportType
{
    UART,
    SPI,
    USB_CDC,
};

struct Stm32TransportConfig
{
    Stm32TransportType type{Stm32TransportType::UART};
    std::string devicePath;
    std::string pinmuxPath;
    int baudRate{0};
    int spiClockHz{0};
    int timeoutMs{0};
    bool mockMode{false};
};

struct Stm32ProtocolFrame
{
    std::uint8_t command{0};
    std::vector<std::uint8_t> payload;
    std::uint8_t flags{0};
};

struct Stm32ProbeResult
{
    bool hostDevicePresent{false};
    bool pinmuxConfigured{false};
    bool transportReady{false};
    bool mockMode{false};
    std::string summary;
};

struct Stm32RoundTripResult
{
    hal::Status status{hal::Status::ERROR};
    std::string detail;
    std::vector<std::uint8_t> response;
    int latencyUs{0};
};

class Stm32Transport
{
  public:
    explicit Stm32Transport(Stm32TransportConfig config = fromEnvironment());

    static Stm32TransportConfig fromEnvironment();
    static std::vector<std::uint8_t> encodeFrame(const Stm32ProtocolFrame& frame);
    static bool decodeFrame(const std::vector<std::uint8_t>& raw, Stm32ProtocolFrame& frameOut);
    static std::vector<std::string> firmwareContract();

    Stm32ProbeResult probe() const;
    Stm32RoundTripResult roundTrip(const Stm32ProtocolFrame& frame) const;
    std::string describe() const;
    const Stm32TransportConfig& config() const;

  private:
    bool hostDevicePresent() const;
    bool pinmuxConfigured() const;
    bool requiresPinmux() const;
    static std::uint8_t checksum(const std::vector<std::uint8_t>& bytes);

    Stm32TransportConfig config_;
};