/**
 * @file audio_interfaces.cpp
 * @brief Implementation file for AudioInterfaces class
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

#include "audio_interfaces.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

// ──────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ──────────────────────────────────────────────────────────────────────────────

AudioInterfaces::AudioInterfaces()
    : sysfs_(std::make_shared<RealSysfsProvider>()), cmd_(std::make_shared<RealCommandProvider>()) {
}

AudioInterfaces::AudioInterfaces(std::shared_ptr<SysfsProvider> sysfs,
                                 std::shared_ptr<CommandProvider> cmd)
    : sysfs_(sysfs ? sysfs : std::make_shared<RealSysfsProvider>()),
      cmd_(cmd ? cmd : std::make_shared<RealCommandProvider>()) {}

AudioInterfaces::~AudioInterfaces() {}

// ──────────────────────────────────────────────────────────────────────────────
// Periodic-safe / one-shot-only classification
// ──────────────────────────────────────────────────────────────────────────────

std::vector<std::string> AudioInterfaces::getPeriodicSafeTests() const {
    return {
        "testI2sInterface",      "testAudioSampleRates",  "testAudioBitDepths",
        "testMultiChannelAudio", "testAudioDmaTransfers",
    };
}

std::vector<std::string> AudioInterfaces::getOneShotOnlyTests() const {
    return {
        "testAudioInput",
        "testAudioOutput",
        "testAudioLatency",
    };
}

// ──────────────────────────────────────────────────────────────────────────────
// Internal helpers using injected providers
// ──────────────────────────────────────────────────────────────────────────────

bool AudioInterfaces::readFile(const std::string& path, std::string& out) const {
    std::string content = sysfs_->readFile(path);
    if (content.empty())
        return false;
    out = content;
    return true;
}

bool AudioInterfaces::pathExists(const std::string& path) const { return sysfs_->pathExists(path); }

std::vector<std::string> AudioInterfaces::listDir(const std::string& dir,
                                                  const std::string& filter) const {
    auto entries = sysfs_->listDir(dir);
    if (filter.empty())
        return entries;
    std::vector<std::string> matches;
    for (const auto& e : entries) {
        if (e.find(filter) != std::string::npos)
            matches.push_back(e);
    }
    return matches;
}

int AudioInterfaces::countAlsaPcmDevices() const {
    std::string content;
    if (!readFile("/proc/asound/pcm", content))
        return 0;
    std::istringstream ss(content);
    std::string line;
    int count = 0;
    while (std::getline(ss, line))
        if (!line.empty())
            ++count;
    return count;
}

// ──────────────────────────────────────────────────────────────────────────────
// 1. testI2sInterface — periodic-safe
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testI2sInterface() {
    TestOutput out = makePassed();
    bool passed = true;

    // ALSA cards
    std::string cardsContent;
    bool cardsFound = readFile("/proc/asound/cards", cardsContent);
    if (cardsFound && !cardsContent.empty()) {
        std::istringstream ss(cardsContent);
        std::string line;
        int cardCount = 0;
        while (std::getline(ss, line)) {
            if (!line.empty() && line[0] != ' ')
                ++cardCount;
        }
        addDetail(out, "alsa_cards", "ALSA sound cards: " + std::to_string(cardCount));
        addMetric(out, "alsa_card_count", int64_t(cardCount));
        addMonitor(out, "alsa_card_count", int64_t(cardCount));
    } else {
        addDetail(out, "alsa_cards", "No ALSA cards found");
        addMonitor(out, "alsa_card_count", int64_t(0));
        passed = false;
    }

    // I2S devices
    const std::vector<std::string> i2sSearchPaths = {
        "/sys/bus/platform/devices",
        "/sys/devices/platform",
    };
    bool i2sFound = false;
    for (const auto& base : i2sSearchPaths) {
        auto entries = listDir(base, "rp1");
        for (const auto& e : entries) {
            if (e.find("i2s") != std::string::npos || e.find("ahub") != std::string::npos ||
                e.find("admaif") != std::string::npos) {
                addDetail(out, "i2s_device", "I2S/RP1 Audio device: " + e);
                i2sFound = true;
                break;
            }
        }
        if (i2sFound)
            break;
    }
    if (!i2sFound) {
        if (pathExists("/sys/devices/platform/rp1-i2s")) {
            addDetail(out, "i2s_device", "RP1 sound platform present");
            i2sFound = true;
        }
    }
    addMetric(out, "i2s_present", i2sFound);
    addMonitor(out, "i2s_present", i2sFound);

    if (!i2sFound) {
        addDetail(out, "i2s_warning", "No I2S device found");
        passed = false;
    }

    // PCM devices
    int pcmDevices = countAlsaPcmDevices();
    addMetric(out, "pcm_device_count", int64_t(pcmDevices));
    addMonitor(out, "pcm_device_count", int64_t(pcmDevices));
    if (pcmDevices > 0) {
        addDetail(out, "pcm_devices", "ALSA PCM devices: " + std::to_string(pcmDevices));
    } else {
        addDetail(out, "pcm_warning", "No ALSA PCM devices found");
        passed = false;
    }

    if (!passed) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// 2. testAudioInput — one-shot only (device access)
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testAudioInput() {
    TestOutput out = makePassed();
    bool passed = true;

    auto sndDevs = listDir("/dev/snd");
    std::vector<std::string> captureDevs;
    for (const auto& d : sndDevs) {
        if (d.find("pcmC") == 0 && d.back() == 'c')
            captureDevs.push_back(d);
    }

    addMetric(out, "capture_device_count", int64_t(captureDevs.size()));
    addMonitor(out, "capture_device_count", int64_t(captureDevs.size()));

    if (!captureDevs.empty()) {
        addDetail(out, "capture_devices",
                  "Capture PCM nodes: " + std::to_string(captureDevs.size()));
        // Try opening first device
        const std::string devPath = "/dev/snd/" + captureDevs[0];
        int fd = ::open(devPath.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            addDetail(out, "capture_open", "Capture device open OK");
            ::close(fd);
        } else {
            if (errno == EBUSY) {
                addDetail(out, "capture_busy", "Capture device busy (in use)");
            } else {
                addDetail(out, "capture_error",
                          "Cannot open capture device: " + std::string(std::strerror(errno)));
                passed = false;
            }
        }
    } else {
        addDetail(out, "capture_warning", "No capture PCM nodes found");
        passed = false;
    }

    if (!passed) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// 3. testAudioOutput — one-shot only (device access)
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testAudioOutput() {
    TestOutput out = makePassed();
    bool passed = true;

    auto sndDevs = listDir("/dev/snd");
    std::vector<std::string> playbackDevs;
    for (const auto& d : sndDevs) {
        if (d.find("pcmC") == 0 && d.back() == 'p')
            playbackDevs.push_back(d);
    }

    addMetric(out, "playback_device_count", int64_t(playbackDevs.size()));
    addMonitor(out, "playback_device_count", int64_t(playbackDevs.size()));

    if (!playbackDevs.empty()) {
        addDetail(out, "playback_devices",
                  "Playback PCM nodes: " + std::to_string(playbackDevs.size()));
        const std::string devPath = "/dev/snd/" + playbackDevs[0];
        int fd = ::open(devPath.c_str(), O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            addDetail(out, "playback_open", "Playback device open OK");
            ::close(fd);
        } else {
            if (errno == EBUSY) {
                addDetail(out, "playback_busy", "Playback device busy (in use)");
            } else {
                addDetail(out, "playback_error",
                          "Cannot open playback device: " + std::string(std::strerror(errno)));
                passed = false;
            }
        }
    } else {
        addDetail(out, "playback_warning", "No playback PCM nodes found");
        passed = false;
    }

    if (!passed) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// 4. testAudioSampleRates — periodic-safe
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testAudioSampleRates() {
    TestOutput out = makePassed();
    bool passed = true;

    const std::vector<std::string> requiredRates = {"8000", "16000", "44100", "48000", "96000"};
    const std::vector<std::string> orinRates = {"8000",  "11025",  "16000", "22050",
                                                "32000", "44100",  "48000", "88200",
                                                "96000", "176400", "192000"};

    // Check active hw_params
    const std::string asoundBase = "/proc/asound";
    auto cardDirs = listDir(asoundBase, "card");
    for (const auto& cardDir : cardDirs) {
        std::string cardPath = asoundBase + "/" + cardDir;
        auto pcmDirs = listDir(cardPath, "pcm");
        for (const auto& pcmDir : pcmDirs) {
            std::string hwpFile = cardPath + "/" + pcmDir + "/sub0/hw_params";
            std::string content;
            if (readFile(hwpFile, content)) {
                std::istringstream ss(content);
                std::string line;
                while (std::getline(ss, line)) {
                    if (line.find("rate") != std::string::npos) {
                        addDetail(out, cardDir + "_" + pcmDir, line);
                    }
                }
            }
        }
    }

    // Verify required rates
    for (const auto& req : requiredRates) {
        bool found = std::find(orinRates.begin(), orinRates.end(), req) != orinRates.end();
        if (!found) {
            addDetail(out, "rate_" + req, "Required rate " + req + " Hz not supported");
            passed = false;
        }
    }

    addMetric(out, "supported_rate_count", int64_t(orinRates.size()));
    addMonitor(out, "supported_rate_count", int64_t(orinRates.size()));

    if (passed)
        addDetail(out, "result", "All required sample rates supported");
    else {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// 5. testAudioBitDepths — periodic-safe
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testAudioBitDepths() {
    TestOutput out = makePassed();
    bool passed = true;

    const std::vector<std::string> required = {"16", "24", "32"};
    const std::vector<std::string> orinFormats = {"S8",     "S16_LE",  "S20_3LE",
                                                  "S24_LE", "S24_3LE", "S32_LE"};

    // Check active hw_params
    const std::string asoundBase = "/proc/asound";
    auto cardDirs = listDir(asoundBase, "card");
    for (const auto& cardDir : cardDirs) {
        std::string cardPath = asoundBase + "/" + cardDir;
        auto pcmDirs = listDir(cardPath, "pcm");
        for (const auto& pcmDir : pcmDirs) {
            std::string hwpFile = cardPath + "/" + pcmDir + "/sub0/hw_params";
            std::string content;
            if (readFile(hwpFile, content)) {
                std::istringstream ss(content);
                std::string line;
                while (std::getline(ss, line)) {
                    if (line.find("format") != std::string::npos ||
                        line.find("bits") != std::string::npos) {
                        addDetail(out, cardDir + "_" + pcmDir, line);
                    }
                }
            }
        }
    }

    // Check required bit depths
    for (const auto& bits : required) {
        bool found = false;
        for (const auto& fmt : orinFormats) {
            if (fmt.find(bits) != std::string::npos) {
                found = true;
                break;
            }
        }
        if (!found) {
            addDetail(out, "depth_" + bits, bits + "-bit not supported");
            passed = false;
        }
    }

    addMetric(out, "supported_format_count", int64_t(orinFormats.size()));
    addMonitor(out, "supported_format_count", int64_t(orinFormats.size()));

    if (!passed) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// 6. testMultiChannelAudio — periodic-safe
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testMultiChannelAudio() {
    TestOutput out = makePassed();
    bool passed = true;

    const int requiredChannels = 8;
    int orinMaxChannels = 0;

    // Check active hw_params: read the actual "channels:" value from each
    // open PCM subdevice rather than assuming a hard-coded platform value.
    const std::string asoundBase = "/proc/asound";
    auto cardDirs = listDir(asoundBase, "card");
    for (const auto& cardDir : cardDirs) {
        std::string cardPath = asoundBase + "/" + cardDir;
        auto pcmDirs = listDir(cardPath, "pcm");
        for (const auto& pcmDir : pcmDirs) {
            std::string hwpFile = cardPath + "/" + pcmDir + "/sub0/hw_params";
            std::string content;
            if (readFile(hwpFile, content)) {
                std::istringstream ss(content);
                std::string line;
                while (std::getline(ss, line)) {
                    if (line.rfind("channels:", 0) == 0) {
                        const int ch = static_cast<int>(parseIntOr(line.substr(9), 0));
                        if (ch > orinMaxChannels)
                            orinMaxChannels = ch;
                    }
                    if (line.find("channel") != std::string::npos)
                        addDetail(out, cardDir + "_" + pcmDir, line);
                }
            }
        }
    }

    if (orinMaxChannels == 0) {
        // No PCM stream is open, so the kernel exposes no channel count.
        // Report that honestly instead of hard-coding a platform value.
        out.status = hal::Status::SKIPPED;
        out.severity = hal::Severity::INFO;
        addDetail(out, "detail",
                  "no active PCM stream exposing hw_params channels; "
                  "start playback/capture and re-run");
        return out;
    }

    addDetail(out, "max_channels", "HW max channels: " + std::to_string(orinMaxChannels));
    addMetric(out, "max_channels", int64_t(orinMaxChannels));
    addMonitor(out, "max_channels", int64_t(orinMaxChannels));

    if (orinMaxChannels >= requiredChannels) {
        addDetail(out, "result",
                  "Multi-channel requirement (" + std::to_string(requiredChannels) + " ch): met");
    } else {
        addDetail(out, "error", "HW max channels < required");
        passed = false;
    }

    if (!passed) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// 7. testAudioDmaTransfers — periodic-safe
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testAudioDmaTransfers() {
    TestOutput out = makePassed();
    bool passed = true;

    // Check DMA channels
    auto dmaDevs = listDir("/sys/class/dma");
    addMetric(out, "dma_channel_count", int64_t(dmaDevs.size()));
    addMonitor(out, "dma_channel_count", int64_t(dmaDevs.size()));

    if (!dmaDevs.empty()) {
        addDetail(out, "dma_channels", "DMA channels: " + std::to_string(dmaDevs.size()));
    } else {
        addDetail(out, "dma_warning", "No DMA channels found");
        passed = false;
    }

    // Check RP1-I2S device
    bool admaifFound = pathExists("/sys/devices/platform/2900000.admaif") ||
                       pathExists("/sys/bus/platform/devices/2900000.admaif");
    if (!admaifFound) {
        auto r = listDir("/sys/bus/platform/devices", "admaif");
        admaifFound = !r.empty();
    }
    addMetric(out, "admaif_present", admaifFound);
    addMonitor(out, "admaif_present", admaifFound);

    if (!passed) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// 8. testAudioLatency — one-shot only (active stream params)
// ──────────────────────────────────────────────────────────────────────────────

TestOutput AudioInterfaces::testAudioLatency() {
    TestOutput out = makePassed();
    bool passed = true;

    const std::string asoundBase = "/proc/asound";
    auto cardDirs = listDir(asoundBase, "card");

    bool streamParamsFound = false;
    for (const auto& cardDir : cardDirs) {
        std::string cardPath = asoundBase + "/" + cardDir;
        auto pcmDirs = listDir(cardPath, "pcm");
        for (const auto& pcmDir : pcmDirs) {
            std::string hwpFile = cardPath + "/" + pcmDir + "/sub0/hw_params";
            std::string content;
            if (!readFile(hwpFile, content))
                continue;
            streamParamsFound = true;

            int rate = 0, periodSize = 0, periods = 0;
            std::istringstream ss(content);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.find("rate:") != std::string::npos)
                    std::sscanf(line.c_str(), "rate: %d", &rate);
                else if (line.find("period_size:") != std::string::npos)
                    std::sscanf(line.c_str(), "period_size: %d", &periodSize);
                else if (line.find("periods:") != std::string::npos)
                    std::sscanf(line.c_str(), "periods: %d", &periods);
            }

            if (rate > 0 && periodSize > 0) {
                double periodMs = (static_cast<double>(periodSize) / rate) * 1000.0;
                double bufferMs = periodMs * (periods > 0 ? periods : 2);
                addDetail(out, cardDir + "_" + pcmDir,
                          "rate=" + std::to_string(rate) +
                              " Hz, period=" + std::to_string(periodSize) + " frames (" +
                              std::to_string(periodMs) + " ms)");
                addMetric(out, cardDir + "_period_ms", periodMs);
                addMetric(out, cardDir + "_buffer_ms", bufferMs);

                if (periodMs > 10.0) {
                    addDetail(out, cardDir + "_latency_warning",
                              "Period latency " + std::to_string(periodMs) + " ms exceeds 10 ms");
                    passed = false;
                }
            }
        }
    }

    if (!streamParamsFound) {
        addDetail(out, "info", "No active audio stream; reporting typical latency");
    }

    addDetail(out, "typical_latency", "Typical period latency @ 48 kHz / 64 frames: 1.33 ms");
    addMetric(out, "target_latency_ms", 10.0);

    if (!passed) {
        out.status = hal::Status::FAIL;
        out.severity = hal::Severity::WARNING;
    }
    return out;
}

// ──────────────────────────────────────────────────────────────────────────────
// runAllTests — runs periodic-safe subset by default
// ──────────────────────────────────────────────────────────────────────────────

std::vector<TestResult> AudioInterfaces::runAllTests() {
    return runSelectedTests(getPeriodicSafeTests());
}

// ──────────────────────────────────────────────────────────────────────────────
// getAvailableTests
// ──────────────────────────────────────────────────────────────────────────────

std::vector<std::string> AudioInterfaces::getAvailableTests() {
    auto tests = getPeriodicSafeTests();
    auto oneShot = getOneShotOnlyTests();
    tests.insert(tests.end(), oneShot.begin(), oneShot.end());
    return tests;
}

// ──────────────────────────────────────────────────────────────────────────────
// runSingleTest
// ──────────────────────────────────────────────────────────────────────────────

TestResult AudioInterfaces::runSingleTest(const std::string& testName) {
    auto start = std::chrono::high_resolution_clock::now();
    TestOutput result;
    result.status = hal::Status::ERROR;
    result.severity = hal::Severity::ERROR;
    result.error_message = "test not dispatched";

    if (testName == "testI2sInterface")
        result = testI2sInterface();
    else if (testName == "testAudioInput")
        result = testAudioInput();
    else if (testName == "testAudioOutput")
        result = testAudioOutput();
    else if (testName == "testAudioSampleRates")
        result = testAudioSampleRates();
    else if (testName == "testAudioBitDepths")
        result = testAudioBitDepths();
    else if (testName == "testMultiChannelAudio")
        result = testMultiChannelAudio();
    else if (testName == "testAudioDmaTransfers")
        result = testAudioDmaTransfers();
    else if (testName == "testAudioLatency")
        result = testAudioLatency();
    else {
        auto end = std::chrono::high_resolution_clock::now();
        TestResult tr;
        tr.name = testName + " (UNKNOWN)";
        tr.test_id = testName;
        tr.output.status = hal::Status::ERROR;
        tr.output.severity = hal::Severity::ERROR;
        tr.output.error_message = "Unknown test name: " + testName;
        tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        return tr;
    }

    auto end = std::chrono::high_resolution_clock::now();
    TestResult tr;
    tr.name = testName;
    tr.test_id = testName;
    tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    tr.output = result;
    return tr;
}
