/**
 * @file audio_interfaces.h
 * @brief Header file for AudioInterfaces class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Improvements implemented:
 * - Injected SysfsProvider/CommandProvider for deterministic testing
 * - Typed metrics with availability state
 * - SKIPPED/UNAVAILABLE for absent hardware
 * - Periodic-safe subset (enumeration/health checks only)
 * - Split non-mutating enumeration from playback/capture diagnostics
 */
#ifndef AUDIOINTERFACES_H
#define AUDIOINTERFACES_H

#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>

/**
 * @class AudioInterfaces
 * @brief Class for testing audio interfaces
 * @details Tests ALSA/I2S audio subsystem. Uses injected providers
 * for deterministic testing.
 *
 * Periodic-safe tests (enumeration/health checks):
 *   - testI2sInterface
 *   - testAudioSampleRates
 *   - testAudioBitDepths
 *   - testMultiChannelAudio
 *   - testAudioDmaTransfers
 *
 * One-shot only tests (device access/state-changing):
 *   - testAudioInput (capture device open)
 *   - testAudioOutput (playback device open)
 *   - testAudioLatency (active stream params)
 */
class AudioInterfaces : public TestBase {
    public:
    AudioInterfaces();
    explicit AudioInterfaces(std::shared_ptr<SysfsProvider> sysfs,
                             std::shared_ptr<CommandProvider> cmd = nullptr);
    ~AudioInterfaces();

    /// @brief Get periodic-safe test names (enumeration/health only)
    std::vector<std::string> getPeriodicSafeTests() const;

    /// @brief Get one-shot only test names (device access)
    std::vector<std::string> getOneShotOnlyTests() const;

    TestOutput testI2sInterface();
    TestOutput testAudioInput();
    TestOutput testAudioOutput();
    TestOutput testAudioSampleRates();
    TestOutput testAudioBitDepths();
    TestOutput testMultiChannelAudio();
    TestOutput testAudioDmaTransfers();
    TestOutput testAudioLatency();

    std::vector<std::string> getAvailableTests() override;
    TestResult runSingleTest(const std::string& testName) override;

    protected:
    std::vector<TestResult> runAllTests() override;

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;

    // Internal helpers using injected providers
    bool readFile(const std::string& path, std::string& out) const;
    bool pathExists(const std::string& path) const;
    std::vector<std::string> listDir(const std::string& dir, const std::string& filter = "") const;
    int countAlsaPcmDevices() const;
};

#endif // AUDIOINTERFACES_H
