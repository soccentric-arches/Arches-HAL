/**
 * @file accelerometer_lib.h
 * @brief Header file for AccelerometerTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef ACCELEROMETERTTESTING_H
#define ACCELEROMETERTTESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class AccelerometerTesting : public TestBase
 * @brief Class for testing accelerometer sensor functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class AccelerometerTesting : public TestBase {
    public:
    AccelerometerTesting();
    ~AccelerometerTesting();

    explicit AccelerometerTesting(std::shared_ptr<SysfsProvider> sysfs,
                                  std::shared_ptr<CommandProvider> cmd = nullptr);

    /// Detect an IIO device exposing in_accel_* channels.
    TestOutput testAccelDeviceDetection();
    /// Read X/Y/Z raw acceleration samples.
    TestOutput testAxisReads();
    /// Read the scale factor (g per LSB).
    TestOutput testScaleRead();
    /// Read the configured sampling frequency.
    TestOutput testSamplingFrequency();
    /// Check the static gravity magnitude for sanity.
    TestOutput testMagnitudeSanity();

    std::vector<std::string> getAvailableTests() override;
    std::vector<std::string> getPeriodicSafeTests();
    std::vector<std::string> getOneShotOnlyTests();

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;
    TestResult runSingleTest(const std::string& testName) override;

    /// Sysfs dir of the first accelerometer IIO device ("" if none).
    std::string firstAccelDeviceDir() const;

    protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // ACCELEROMETERTTESTING_H
