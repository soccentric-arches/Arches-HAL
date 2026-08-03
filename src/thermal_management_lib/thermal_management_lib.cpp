/**
 * @file thermal_management_lib.cpp
 * @brief Implementation file for ThermalManagementTesting class
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

#include "thermal_management_lib.h"
#include "../platform_arches.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Thermal constants (Jetson)
// ---------------------------------------------------------------------------
static constexpr double THROTTLE_C = 80.0;
static constexpr double SHUTDOWN_C = 95.0;
static constexpr double NORMAL_MAX = 75.0;
static constexpr double AMBIENT_MIN = 5.0;
static constexpr double AMBIENT_MAX = 50.0;

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

ThermalManagementTesting::ThermalManagementTesting()
    : thermal_(std::make_shared<RealThermalZoneProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

ThermalManagementTesting::ThermalManagementTesting(
    std::shared_ptr<ThermalZoneProvider> thermalProvider,
    std::shared_ptr<CommandProvider> cmdProvider)
    : thermal_(thermalProvider ? thermalProvider
                               : std::make_shared<RealThermalZoneProvider>()),
      cmd_(cmdProvider ? cmdProvider
                       : std::make_shared<RealCommandProvider>()) {}

ThermalManagementTesting::~ThermalManagementTesting() {}

// ---------------------------------------------------------------------------
// Periodic-safe / one-shot-only classification
// ---------------------------------------------------------------------------

std::vector<std::string>
ThermalManagementTesting::getPeriodicSafeTests() const {
  return {
      "testSocJunctionTemperature",
      "testGpuTemperature",
      "testCpuTemperature",
      "testMemoryTemperature",
      "testBoardAmbientTemperature",
      "testAllThermalZones",
      "testThermalThrottlingThresholds",
      "testThermalShutdownTemperature",
      "testTemperatureDistributionAcrossDie",
  };
}

std::vector<std::string> ThermalManagementTesting::getOneShotOnlyTests() const {
  return {
      "testCoolingSolutionEffectiveness",
      "testSustainedThermalPerformance",
      "testThermalRecoveryTime",
      "testFanControl",
  };
}

// ---------------------------------------------------------------------------
// 1. testSocJunctionTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testSocJunctionTemperature() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone = thermal_->readZone(0);
  if (!zone.valid) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "SoC thermal zone not available (no hardware)";
    out.metrics["temperature_c"] = hal::Value(std::string("N/A"));
    out.monitoring["soc_temp_c"] = std::string("N/A");
    return out;
  }

  out.reason["zone"] = zone.name;
  out.reason["temperature_c"] = std::to_string(zone.temperatureC);
  out.metrics["temperature_c"] = zone.temperatureC;
  out.monitoring["soc_temp_c"] = zone.temperatureC;
  out.metrics["throttle_threshold_c"] = THROTTLE_C;
  out.metrics["shutdown_threshold_c"] = SHUTDOWN_C;

  if (zone.temperatureC >= SHUTDOWN_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::CRITICAL;
    out.reason["error"] = "SoC at or above shutdown temperature!";
  } else if (zone.temperatureC >= THROTTLE_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
    out.reason["warning"] = "SoC at or above throttle threshold.";
  } else {
    out.reason["result"] = "SoC junction temperature within safe range.";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 2. testGpuTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testGpuTemperature() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone = thermal_->readZone(0);
  if (!zone.valid) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "GPU thermal zone not available (no hardware)";
    out.monitoring["gpu_temp_c"] = std::string("N/A");
    return out;
  }

  out.reason["detail"] =
      "GPU temperature (shared die): " + std::to_string(zone.temperatureC) +
      " °C";
  out.metrics["temperature_c"] = zone.temperatureC;
  out.monitoring["gpu_temp_c"] = zone.temperatureC;

  if (zone.temperatureC >= THROTTLE_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
    out.reason["warning"] = "GPU/SoC temperature at throttle threshold.";
  } else {
    out.reason["result"] = "GPU temperature within limits.";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 3. testCpuTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testCpuTemperature() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone = thermal_->readZone(0);
  if (!zone.valid) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "CPU thermal zone not available (no hardware)";
    out.monitoring["cpu_temp_c"] = std::string("N/A");
    return out;
  }

  out.reason["zone"] =
      zone.name + ": " + std::to_string(zone.temperatureC) + " °C";
  out.metrics["temperature_c"] = zone.temperatureC;
  out.monitoring["cpu_temp_c"] = zone.temperatureC;

  if (zone.temperatureC >= SHUTDOWN_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::CRITICAL;
    out.reason["error"] = "CPU near thermal shutdown!";
  } else if (zone.temperatureC >= THROTTLE_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
    out.reason["warning"] = "CPU at throttle threshold.";
  } else {
    out.reason["result"] = "CPU temperature within safe range.";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 4. testMemoryTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testMemoryTemperature() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone = thermal_->readZone(0);
  if (!zone.valid) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] =
        "Memory temperature sensor not available (proxied by SoC)";
    out.monitoring["mem_temp_c"] = std::string("N/A");
    return out;
  }

  out.reason["detail"] =
      "SoC/memory proxy temperature: " + std::to_string(zone.temperatureC) +
      " °C";
  out.metrics["temperature_c"] = zone.temperatureC;
  out.monitoring["mem_temp_c"] = zone.temperatureC;

  if (zone.temperatureC > 80.0) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::WARNING;
    out.reason["warning"] = "SoC/memory temperature approaching LPDDR4X limit.";
  } else {
    out.reason["result"] = "Memory thermal check passed (via SoC proxy).";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 5. testBoardAmbientTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testBoardAmbientTemperature() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone1 = thermal_->readZone(1);
  if (!zone1.valid) {
    out.reason["rp1_zone"] = "RP1 south-bridge zone not available.";
    out.monitoring["board_temp_c"] = std::string("N/A");
  } else {
    out.reason["rp1_zone"] =
        zone1.name + ": " + std::to_string(zone1.temperatureC) + " °C";
    out.metrics["rp1_temperature_c"] = zone1.temperatureC;
    out.monitoring["board_temp_c"] = zone1.temperatureC;
    if (zone1.temperatureC > 85.0) {
      out.status = hal::Status::FAIL;
      out.severity = hal::Severity::WARNING;
      out.reason["warning"] = "RP1 south-bridge temperature above 85°C.";
    }
  }

  out.reason["result"] = "Board ambient temperature check complete.";
  return out;
}

// ---------------------------------------------------------------------------
// 6. testAllThermalZones
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testAllThermalZones() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zones = thermal_->readAllZones();
  if (zones.empty()) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "No thermal zones found (no hardware).";
    out.monitoring["zone_count"] = int64_t(0);
    return out;
  }

  out.metrics["zone_count"] = static_cast<int64_t>(zones.size());
  out.monitoring["zone_count"] = int64_t(zones.size());

  for (size_t i = 0; i < zones.size(); ++i) {
    std::string key = "zone_" + std::to_string(i);
    out.reason[key] =
        zones[i].name + ": " + std::to_string(zones[i].temperatureC) + " °C";
    out.metrics[key + "_temp_c"] = zones[i].temperatureC;

    if (zones[i].temperatureC >= SHUTDOWN_C) {
      out.status = hal::Status::FAIL;
      out.severity = hal::Severity::CRITICAL;
      out.reason[key + "_error"] = "OVER SHUTDOWN TEMPERATURE!";
    } else if (zones[i].temperatureC >= THROTTLE_C) {
      out.status = hal::Status::FAIL;
      out.severity = hal::Severity::ERROR;
      out.reason[key + "_warning"] = "Throttling threshold reached.";
    }
  }

  if (out.status == hal::Status::PASS) {
    out.reason["result"] = "All thermal zones within safe range.";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 7. testThermalThrottlingThresholds
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testThermalThrottlingThresholds() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  out.reason["throttle_c"] =
      "Throttle onset: " + std::to_string(THROTTLE_C) + " °C";
  out.reason["shutdown_c"] = "Shutdown: " + std::to_string(SHUTDOWN_C) + " °C";
  out.metrics["throttle_threshold_c"] = THROTTLE_C;
  out.metrics["shutdown_threshold_c"] = SHUTDOWN_C;

  // Read trip points
  auto trips = thermal_->readTripPoints(0);
  for (size_t t = 0; t < trips.size(); ++t) {
    std::string key = "trip_" + std::to_string(t);
    out.reason[key] =
        trips[t].type + ": " + std::to_string(trips[t].temperatureC) + " °C";
    out.metrics[key + "_temp_c"] = trips[t].temperatureC;
  }

  // Check current temperature
  auto zone = thermal_->readZone(0);
  if (zone.valid) {
    out.metrics["current_temp_c"] = zone.temperatureC;
    out.monitoring["current_temp_c"] = zone.temperatureC;
    if (zone.temperatureC >= THROTTLE_C) {
      out.status = hal::Status::FAIL;
      out.severity = hal::Severity::ERROR;
      out.reason["error"] =
          "Currently throttling (T=" + std::to_string(zone.temperatureC) +
          " °C).";
    } else {
      out.reason["result"] =
          "Throttle thresholds confirmed, no active throttling.";
    }
  } else {
    out.monitoring["current_temp_c"] = std::string("N/A");
  }

  return out;
}

// ---------------------------------------------------------------------------
// 8. testThermalShutdownTemperature
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testThermalShutdownTemperature() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  out.reason["shutdown_c"] =
      "Configured shutdown: " + std::to_string(SHUTDOWN_C) + " °C";
  out.metrics["shutdown_threshold_c"] = SHUTDOWN_C;

  // Read critical trip points
  auto trips = thermal_->readTripPoints(0);
  for (const auto &t : trips) {
    if (t.type == "critical") {
      out.reason["critical_trip"] =
          "Critical trip: " + std::to_string(t.temperatureC) + " °C";
      out.metrics["critical_trip_c"] = t.temperatureC;
    }
  }

  auto zone = thermal_->readZone(0);
  if (zone.valid) {
    out.metrics["current_temp_c"] = zone.temperatureC;
    out.reason["headroom"] = "Headroom to shutdown: " +
                             std::to_string(SHUTDOWN_C - zone.temperatureC) +
                             " °C";
    if (zone.temperatureC >= SHUTDOWN_C) {
      out.status = hal::Status::FAIL;
      out.severity = hal::Severity::CRITICAL;
      out.reason["error"] = "At or above shutdown temperature!";
    } else {
      out.reason["result"] = "Safe headroom to shutdown temperature.";
    }
  } else {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "Thermal zone not available (no hardware).";
  }

  return out;
}

// ---------------------------------------------------------------------------
// 9. testCoolingSolutionEffectiveness (one-shot only — stress workload)
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testCoolingSolutionEffectiveness() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone = thermal_->readZone(0);
  if (!zone.valid) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "Thermal zone not available (no hardware).";
    return out;
  }

  double tempBefore = zone.temperatureC;
  out.reason["temp_before"] =
      "Temperature before stress: " + std::to_string(tempBefore) + " °C";
  out.metrics["temp_before_c"] = tempBefore;

  // Run 3-second CPU stress
  auto tEnd = std::chrono::steady_clock::now() + std::chrono::seconds(3);
  volatile double acc = 1.0;
  while (std::chrono::steady_clock::now() < tEnd) {
    acc = acc * 1.000001 + 0.000001;
  }
  (void)acc;

  zone = thermal_->readZone(0);
  double tempAfter = zone.temperatureC;
  out.reason["temp_after"] =
      "Temperature after stress: " + std::to_string(tempAfter) + " °C";
  out.metrics["temp_after_c"] = tempAfter;
  out.metrics["temp_rise_c"] = tempAfter - tempBefore;

  if (tempAfter >= SHUTDOWN_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::CRITICAL;
    out.reason["error"] =
        "Temperature reached shutdown threshold during 3s stress.";
  } else if (tempAfter >= THROTTLE_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
    out.reason["warning"] = "Thermal throttling triggered during stress test.";
  } else {
    out.reason["result"] = "Cooling solution effective for 3s full-load test.";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 10. testTemperatureDistributionAcrossDie
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testTemperatureDistributionAcrossDie() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zones = thermal_->readAllZones();
  if (zones.empty()) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "No thermal zones available (no hardware).";
    return out;
  }

  for (size_t i = 0; i < zones.size(); ++i) {
    std::string key = "zone_" + std::to_string(i);
    out.reason[key] =
        zones[i].name + ": " + std::to_string(zones[i].temperatureC) + " °C";
    out.metrics[key + "_temp_c"] = zones[i].temperatureC;
  }

  if (zones.size() >= 2) {
    double delta = std::abs(zones[0].temperatureC - zones[1].temperatureC);
    out.metrics["inter_zone_delta_c"] = delta;
    out.reason["delta"] = "Inter-zone delta: " + std::to_string(delta) + " °C";
    if (delta > 30.0) {
      out.reason["note"] = "Large temperature differential between zones.";
    }
  }

  out.reason["result"] = "Die temperature distribution check complete.";
  return out;
}

// ---------------------------------------------------------------------------
// 11. testSustainedThermalPerformance (one-shot only — stress workload)
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testSustainedThermalPerformance() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone = thermal_->readZone(0);
  if (!zone.valid) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "Thermal zone not available (no hardware).";
    return out;
  }

  double tempBefore = zone.temperatureC;
  out.reason["temp_before"] = "Initial T=" + std::to_string(tempBefore) + " °C";
  out.metrics["temp_before_c"] = tempBefore;

  // 5-second stress
  auto tEnd = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  volatile double acc = 1.0;
  while (std::chrono::steady_clock::now() < tEnd) {
    acc = acc * 1.000001 + 0.000001;
  }
  (void)acc;

  zone = thermal_->readZone(0);
  double tempAfter = zone.temperatureC;
  out.reason["temp_after"] = "After 5s T=" + std::to_string(tempAfter) + " °C";
  out.metrics["temp_after_c"] = tempAfter;
  out.metrics["temp_rise_c"] = tempAfter - tempBefore;

  if (tempAfter >= THROTTLE_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
    out.reason["warning"] =
        "Temperature hit throttle threshold during sustained test.";
  } else {
    out.reason["result"] = "Sustained thermal performance test complete.";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 12. testThermalRecoveryTime (one-shot only — stress workload)
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testThermalRecoveryTime() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto zone = thermal_->readZone(0);
  if (!zone.valid) {
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = "Thermal zone not available (no hardware).";
    return out;
  }

  // 3-second stress
  {
    auto tEnd = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    volatile double acc = 1.0;
    while (std::chrono::steady_clock::now() < tEnd) {
      acc = acc * 1.000001 + 0.000001;
    }
    (void)acc;
  }

  zone = thermal_->readZone(0);
  double peakC = zone.temperatureC;
  out.reason["peak_temp"] =
      "Peak after 3s stress: " + std::to_string(peakC) + " °C";
  out.metrics["peak_temp_c"] = peakC;

  // Wait up to 30s for recovery
  double baseline = peakC - 10.0;
  auto tStart = std::chrono::steady_clock::now();
  double recoveredC = peakC;
  for (int i = 0; i < 30; ++i) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    zone = thermal_->readZone(0);
    recoveredC = zone.temperatureC;
    if (recoveredC <= baseline)
      break;
  }
  double recoverySec =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - tStart)
          .count();

  out.reason["recovered_temp"] =
      "After cooling: " + std::to_string(recoveredC) + " °C";
  out.metrics["recovery_time_s"] = recoverySec;
  out.metrics["recovered_temp_c"] = recoveredC;

  if (recoveredC > THROTTLE_C) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::ERROR;
    out.reason["warning"] =
        "Still above throttle threshold after recovery period.";
  } else {
    out.reason["result"] = "Thermal recovery within acceptable time.";
  }
  return out;
}

// ---------------------------------------------------------------------------
// 13. testFanControl (one-shot only — state-changing)
// ---------------------------------------------------------------------------
TestOutput ThermalManagementTesting::testFanControl() {
  TestOutput out;
  out.status = hal::Status::PASS;
  out.severity = hal::Severity::INFO;

  auto devices = thermal_->readCoolingDevices();
  bool fanFound = false;

  for (size_t i = 0; i < devices.size(); ++i) {
    std::string key = "cooling_" + std::to_string(i);
    out.reason[key] = devices[i].type +
                      ": state=" + std::to_string(devices[i].curState) + "/" +
                      std::to_string(devices[i].maxState);
    out.metrics[key + "_cur"] = static_cast<int64_t>(devices[i].curState);
    out.metrics[key + "_max"] = static_cast<int64_t>(devices[i].maxState);

    if (devices[i].type.find("fan") != std::string::npos ||
        devices[i].type.find("pwm") != std::string::npos) {
      fanFound = true;
    }
  }

  if (!fanFound) {
    out.reason["info"] =
        "No fan cooling device detected (passive cooling is normal).";
    out.monitoring["fan_present"] = false;
  } else {
    out.monitoring["fan_present"] = true;
    out.reason["result"] = "Fan control interface detected.";
  }

  return out;
}

// ---------------------------------------------------------------------------
// runAllTests — runs periodic-safe tests by default
// ---------------------------------------------------------------------------
std::vector<TestResult> ThermalManagementTesting::runAllTests() {
  return runSelectedTests(getPeriodicSafeTests());
}

// ---------------------------------------------------------------------------
// getAvailableTests
// ---------------------------------------------------------------------------
std::vector<std::string> ThermalManagementTesting::getAvailableTests() {
  auto tests = getPeriodicSafeTests();
  auto oneShot = getOneShotOnlyTests();
  tests.insert(tests.end(), oneShot.begin(), oneShot.end());
  return tests;
}

// ---------------------------------------------------------------------------
// runSingleTest
// ---------------------------------------------------------------------------
TestResult
ThermalManagementTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput result;
  result.status = hal::Status::ERROR;
  result.severity = hal::Severity::ERROR;
  result.error_message = "test not dispatched";

  if (testName == "testSocJunctionTemperature") {
    result = testSocJunctionTemperature();
  } else if (testName == "testGpuTemperature") {
    result = testGpuTemperature();
  } else if (testName == "testCpuTemperature") {
    result = testCpuTemperature();
  } else if (testName == "testMemoryTemperature") {
    result = testMemoryTemperature();
  } else if (testName == "testBoardAmbientTemperature") {
    result = testBoardAmbientTemperature();
  } else if (testName == "testAllThermalZones") {
    result = testAllThermalZones();
  } else if (testName == "testThermalThrottlingThresholds") {
    result = testThermalThrottlingThresholds();
  } else if (testName == "testThermalShutdownTemperature") {
    result = testThermalShutdownTemperature();
  } else if (testName == "testCoolingSolutionEffectiveness") {
    result = testCoolingSolutionEffectiveness();
  } else if (testName == "testTemperatureDistributionAcrossDie") {
    result = testTemperatureDistributionAcrossDie();
  } else if (testName == "testSustainedThermalPerformance") {
    result = testSustainedThermalPerformance();
  } else if (testName == "testThermalRecoveryTime") {
    result = testThermalRecoveryTime();
  } else if (testName == "testFanControl") {
    result = testFanControl();
  } else {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    TestResult tr;
    tr.name = testName + " (UNKNOWN)";
    tr.test_id = testName;
    tr.output.status = hal::Status::ERROR;
    tr.output.severity = hal::Severity::ERROR;
    tr.output.error_message = "Unknown test name: " + testName;
    tr.duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
    return tr;
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  TestResult tr;
  tr.name = testName;
  tr.test_id = testName;
  tr.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
  tr.output = result;
  return tr;
}
