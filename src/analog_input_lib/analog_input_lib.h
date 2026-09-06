/**
 * @file analog_input_lib.h
 * @brief Header file for AnalogInputTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef ANALOGINPUTTESTING_H
#define ANALOGINPUTTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class AnalogInputTesting : public TestBase
 * @brief Class for testing ADC analog input functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class AnalogInputTesting : public TestBase {
    public:
    AnalogInputTesting();
    ~AnalogInputTesting();

    explicit AnalogInputTesting(std::shared_ptr<SysfsProvider> sysfs,
                                std::shared_ptr<CommandProvider> cmd = nullptr);

    /// Enumerate IIO devices in /sys/bus/iio/devices.
    TestOutput testIioDeviceEnumeration();
    /// Detect in_voltage*_raw channels on the first IIO device.
    TestOutput testAdcChannelDetection();
    /// Read the first voltage channel raw value.
    TestOutput testRawRead();
    /// Read the channel scale factor.
    TestOutput testScaleRead();
    /// Sample the channel repeatedly and check for a stuck/railing input.
    TestOutput testSaturationCheck();

    std::vector<std::string> getAvailableTests() override;
    std::vector<std::string> getPeriodicSafeTests();
    std::vector<std::string> getOneShotOnlyTests();

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;
    TestResult runSingleTest(const std::string& testName) override;

    /// First IIO device dir containing an in_voltage*_raw file ("" if none).
    std::string firstAdcDeviceDir() const;

    protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // ANALOGINPUTTESTING_H
