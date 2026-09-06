/**
 * @file mipi_csi_lib.h
 * @brief Header file for MipiCsiTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef MIPICSITESTING_H
#define MIPICSITESTING_H
#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>
#include <string>
#include <vector>

/**
 * @class MipiCsiTesting : public TestBase
 * @brief Class for testing MIPI CSI-2 camera interface functionality
 * @details Provider-injected: pass FakeSysfsProvider/FakeCommandProvider for
 * deterministic CI runs; the default constructor uses real sysfs and commands.
 */
class MipiCsiTesting : public TestBase {
    public:
    MipiCsiTesting();
    ~MipiCsiTesting();

    explicit MipiCsiTesting(std::shared_ptr<SysfsProvider> sysfs,
                            std::shared_ptr<CommandProvider> cmd = nullptr);

    /// Enumerate V4L2 device nodes under /dev.
    TestOutput testVideoDeviceEnumeration();
    /// Detect CSI receiver V4L2 subdevices by name.
    TestOutput testCsiReceiverDetection();
    /// Detect image-sensor subdevices by name.
    TestOutput testSensorSubdeviceDetection();
    /// Dump the media controller graph via media-ctl (if installed).
    TestOutput testMediaControllerGraph();
    /// Check for the v4l2-ctl utility and list devices.
    TestOutput testV4l2DeviceNodes();

    std::vector<std::string> getAvailableTests() override;
    std::vector<std::string> getPeriodicSafeTests();
    std::vector<std::string> getOneShotOnlyTests();

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;
    TestResult runSingleTest(const std::string& testName) override;

    protected:
    std::vector<TestResult> runAllTests() override;
};

#endif // MIPICSITESTING_H
