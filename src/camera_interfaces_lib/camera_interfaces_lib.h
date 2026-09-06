/**
 * @file camera_interfaces_lib.h
 * @brief Header file for CameraInterfacesTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Improvements implemented:
 * - Injected SysfsProvider/CommandProvider for deterministic testing
 * - Typed metrics with availability state
 * - SKIPPED/UNAVAILABLE for absent hardware
 * - Periodic-safe subset (enumeration/health checks only)
 * - Split non-mutating enumeration from capture diagnostics
 */
#ifndef CAMERAINTERFACESTESTING_H
#define CAMERAINTERFACESTESTING_H

#include "lib_base.h"
#include "module_helpers.h"
#include "provider_base.h"

#include <memory>

/**
 * @class CameraInterfacesTesting
 * @brief Class for testing camera interfaces
 * @details Tests CSI-2, V4L2, and ISP pipeline. Uses injected providers
 * for deterministic testing.
 *
 * Periodic-safe tests (enumeration/health checks):
 *   - testAllCsi2LanesFunctionality
 *   - testCsi2DphyOperation
 *   - testCsi2CphyOperation
 *   - testVirtualChannelSupport
 *   - testCameraClockGeneration
 *   - testRaw8, testRaw10, testRaw12, testRaw14
 *   - testYuv422, testRgbFormats
 *
 * One-shot only tests (device access/state-changing):
 *   - testMultipleCameraSimultaneousOperation
 *   - testCameraPowerSequencing
 *   - testIspPipelineProcessing
 *   - testAutoExposure, testAutoWhiteBalance, testAutoFocus
 *   - testLensShadingCorrection, testDemosaicing
 *   - testNoiseReduction, testEdgeEnhancement
 *   - testColorCorrectionMatrix, testGammaCorrection
 */
class CameraInterfacesTesting : public TestBase {
    public:
    CameraInterfacesTesting();
    explicit CameraInterfacesTesting(std::shared_ptr<SysfsProvider> sysfs,
                                     std::shared_ptr<CommandProvider> cmd = nullptr);
    ~CameraInterfacesTesting();

    /// @brief Get periodic-safe test names (enumeration/health only)
    std::vector<std::string> getPeriodicSafeTests() const;

    /// @brief Get one-shot only test names (device access/ISP)
    std::vector<std::string> getOneShotOnlyTests() const;

    TestOutput testAllCsi2LanesFunctionality();
    TestOutput testCsi2DphyOperation();
    TestOutput testCsi2CphyOperation();
    TestOutput testMultipleCameraSimultaneousOperation();
    TestOutput testVirtualChannelSupport();
    TestOutput testCameraClockGeneration();
    TestOutput testCameraPowerSequencing();
    TestOutput testRaw8();
    TestOutput testRaw10();
    TestOutput testRaw12();
    TestOutput testRaw14();
    TestOutput testYuv422();
    TestOutput testRgbFormats();
    TestOutput testIspPipelineProcessing();
    TestOutput testAutoExposure();
    TestOutput testAutoWhiteBalance();
    TestOutput testAutoFocus();
    TestOutput testLensShadingCorrection();
    TestOutput testDemosaicing();
    TestOutput testNoiseReduction();
    TestOutput testEdgeEnhancement();
    TestOutput testColorCorrectionMatrix();
    TestOutput testGammaCorrection();

    std::vector<std::string> getAvailableTests() override;
    TestResult runSingleTest(const std::string& testName) override;

    protected:
    std::vector<TestResult> runAllTests() override;

    private:
    std::shared_ptr<SysfsProvider> sysfs_;
    std::shared_ptr<CommandProvider> cmd_;

    // Internal helpers using injected providers
    std::string readFile(const std::string& path) const;
    bool pathExists(const std::string& path) const;
    std::vector<std::string> listDir(const std::string& dir) const;
    bool isModuleLoaded(const std::string& modName) const;
    std::string runCmd(const std::string& cmd) const;
    std::vector<std::string> getVideoDevices() const;
};

#endif // CAMERAINTERFACESTESTING_H
