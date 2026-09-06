/**
 * @file thermal_management_lib.h
 * @brief Header file for ThermalManagementTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Improvements implemented:
 * - Injected ThermalZoneProvider for deterministic testing
 * - Typed metrics with availability state
 * - SKIPPED/UNAVAILABLE for absent hardware
 * - Periodic-safe subset (passive telemetry only)
 * - Split passive telemetry from fan/control/load actions
 */
#ifndef THERMALMANAGEMENTTESTING_H
#define THERMALMANAGEMENTTESTING_H

#include "lib_base.h"
#include "provider_base.h"

#include <memory>

/**
 * @class ThermalManagementTesting
 * @brief Class for testing thermal management functionality
 * @details Tests thermal zones, throttling thresholds, cooling devices,
 * and fan control.  Uses injected providers for deterministic testing.
 *
 * Periodic-safe tests (safe for monitoring/long-term/event modes):
 *   - testSocJunctionTemperature
 *   - testGpuTemperature
 *   - testCpuTemperature
 *   - testMemoryTemperature
 *   - testBoardAmbientTemperature
 *   - testAllThermalZones
 *   - testThermalThrottlingThresholds
 *   - testThermalShutdownTemperature
 *   - testTemperatureDistributionAcrossDie
 *
 * One-shot only tests (destructive/state-changing):
 *   - testCoolingSolutionEffectiveness (stress workload)
 *   - testSustainedThermalPerformance (stress workload)
 *   - testThermalRecoveryTime (stress workload)
 *   - testFanControl (fan state mutation)
 */
class ThermalManagementTesting : public TestBase {
    public:
    ThermalManagementTesting();
    explicit ThermalManagementTesting(std::shared_ptr<ThermalZoneProvider> thermalProvider,
                                      std::shared_ptr<CommandProvider> cmdProvider = nullptr);
    ~ThermalManagementTesting();

    /// @brief Get periodic-safe test names (passive telemetry only)
    std::vector<std::string> getPeriodicSafeTests() const;

    /// @brief Get one-shot only test names (destructive/state-changing)
    std::vector<std::string> getOneShotOnlyTests() const;

    // Test methods
    TestOutput testSocJunctionTemperature();
    TestOutput testGpuTemperature();
    TestOutput testCpuTemperature();
    TestOutput testMemoryTemperature();
    TestOutput testBoardAmbientTemperature();
    TestOutput testAllThermalZones();
    TestOutput testThermalThrottlingThresholds();
    TestOutput testThermalShutdownTemperature();
    TestOutput testCoolingSolutionEffectiveness();
    TestOutput testTemperatureDistributionAcrossDie();
    TestOutput testSustainedThermalPerformance();
    TestOutput testThermalRecoveryTime();
    TestOutput testFanControl();

    std::vector<std::string> getAvailableTests() override;
    TestResult runSingleTest(const std::string& testName) override;

    protected:
    std::vector<TestResult> runAllTests() override;

    private:
    std::shared_ptr<ThermalZoneProvider> thermal_;
    std::shared_ptr<CommandProvider> cmd_;
};

#endif // THERMALMANAGEMENTTESTING_H
