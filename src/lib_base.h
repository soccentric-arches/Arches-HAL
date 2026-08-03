/**
 * @file lib_base.h
 * @brief Base class for all testing classes
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */
#ifndef TEST_BASE_H
#define TEST_BASE_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

/**
 * @enum TestMode
 * @brief Enumeration for test modes
 */
enum class TestMode {

  SINGLE_SHOT = 0, ///< Run all tests once (synchronous)
  MONITORING =
      2,         ///< Run tests repeatedly at an interval until stop() is called
  LONG_TERM = 1, ///< Run tests at an interval for a fixed duration, then stop

  EVENT_BASED =
      3 ///< Run tests continuously, fire a callback when criteria are met
};

// ============================================================
// Canonical result types (hal namespace)
// ============================================================

namespace hal {

/// Typed value used for metrics, monitoring readings, and event payloads.
/// Strings remain available for non-numeric data; numeric/bool data must NOT
/// be stringified — downstream consumers (charting, thresholds) rely on type.
using Value = std::variant<bool, int64_t, double, std::string>;

/// Terminal outcome of a single test execution.
enum class Status {
  PASS,
  FAIL,
  ERROR,
  TIMEOUT,
  SKIPPED,
  ABORTED,
};

/// Severity classifies how serious a non-PASS outcome is, or how informational
/// a PASS is.
enum class Severity {
  INFO,
  WARNING,
  ERROR,
  CRITICAL,
};

/// Output of a single test execution.
struct TestOutput {
  Status status{Status::ERROR};
  Severity severity{Severity::INFO};
  std::map<std::string, std::string> reason;
  std::map<std::string, Value> metrics;
  std::map<std::string, Value> monitoring;
  std::map<std::string, Value> event;
  std::string error_message;
};

/// Result of a single test execution.
struct TestResult {
  std::string module_id;
  std::string test_id;
  std::string name;
  uint64_t sequence{0};
  std::chrono::system_clock::time_point timestamp;
  std::chrono::nanoseconds duration{0};
  TestOutput output;
  std::vector<TestResult> subtests;
};

} // namespace hal

// Bring canonical types into the global namespace for backward compatibility
// with all module headers and implementations.
using hal::Severity;
using hal::Status;
using hal::TestOutput;
using hal::TestResult;
using hal::Value;

/// Availability state for a monitor reading.
enum class MonitorAvailability {
  AVAILABLE,   ///< Value is valid and ready for thresholding
  UNAVAILABLE, ///< Source not found or not present (e.g. missing sysfs file)
  ERROR,       ///< Source found but read failed
};

/// Typed monitor reading with availability state.
/// Numeric values (int64_t, double) are safe for thresholding;
/// string values are used for non-numeric data only.
struct MonitorValue {
  Value value{std::string("")};
  MonitorAvailability availability{MonitorAvailability::UNAVAILABLE};
  std::string error_message;
};

/**
 * @struct MonitorItem
 * @brief A named data source sampled every cycle during a monitoring test
 */
struct MonitorItem {
  std::string name; ///< Display name for this item
  std::function<MonitorValue()>
      reader; ///< Callable that returns a typed value with availability
};

/**
 * @struct MonitoringSnapshot
 * @brief Full snapshot produced each cycle by runMonitoringTest()
 */
struct MonitoringSnapshot {
  int cycle; ///< Cycle count (1-based)
  std::chrono::system_clock::time_point
      timestamp;                       ///< Wall-clock time of snapshot
  std::vector<TestResult> testResults; ///< Results from running all tests
  std::vector<std::pair<std::string, MonitorValue>>
      monitorData; ///< Sampled monitor item values
};

/**
 * @struct EventCriteria
 * @brief Criteria evaluated each cycle by runEventTest()
 */
struct EventCriteria {
  std::string description; ///< Human-readable description
  std::function<bool(const std::vector<TestResult> &)>
      predicate; ///< Returns true when event fires
};

/**
 * @class TestBase
 * @brief Base class for all testing classes
 * @details Provides four test modes:
 *   SINGLE_SHOT       — run all tests once (synchronous)
 *   MONITORING     — run indefinitely at a fixed interval (background thread)
 *   LONG_TERM      — run for a fixed total duration at a fixed interval
 * (background thread) EVENT          — run continuously, fire a callback when
 * criteria are met (background thread)
 *
 * For background modes, call stop() to abort early, or wait for isRunning() to
 * return false.
 */
class TestBase {
public:
  TestBase();
  virtual ~TestBase();

  /**
   * @brief Run tests in SINGLE_SHOT or MONITORING mode (backward-compatible
   * entry point)
   * @param mode        SINGLE_SHOT (synchronous) or MONITORING (starts
   * background thread)
   * @param callback    Called with results each cycle
   * @param interval_ms Cycle interval for MONITORING mode (default 1000 ms)
   */
  void runTests(
      TestMode mode,
      std::function<void(const std::vector<TestResult> &)> callback = nullptr,
      int interval_ms = 1000);

  /**
   * @brief Start a long-term test that runs for a fixed total duration
   * @param interval_ms  Interval between cycles in milliseconds
   * @param duration_ms  Total run time in milliseconds; 0 = run until stop()
   * @param callback     Called after each cycle with results and 1-based cycle
   * number Starts a background thread. Call stop() to abort, or wait for
   * isRunning() == false.
   */
  void runLongTerm(
      int interval_ms, long long duration_ms,
      std::function<void(const std::vector<TestResult> &, int cycle)> callback =
          nullptr);

  /**
   * @brief Start a monitoring test: run tests and sample hardware monitors each
   * cycle
   * @param interval_ms  Interval between cycles in milliseconds
   * @param duration_ms  Total run time in milliseconds; 0 = run until stop()
   * @param monitorItems Additional data sources sampled each cycle
   * @param callback     Called after each cycle with a MonitoringSnapshot
   * Starts a background thread. Call stop() to abort, or wait for isRunning()
   * == false.
   */
  void runMonitoringTest(
      int interval_ms, long long duration_ms,
      const std::vector<MonitorItem> &monitorItems,
      std::function<void(const MonitoringSnapshot &)> callback = nullptr);

  /**
   * @brief Start an event-driven test: fire callback each time criteria are met
   * @param poll_interval_ms  How often to run tests and check the criteria
   * @param criteria          Event criteria (description + predicate)
   * @param event_callback    Called each time criteria.predicate returns true
   * @param max_duration_ms   Stop after this many ms even if criteria never
   * met; 0 = indefinite Starts a background thread. Call stop() to abort, or
   * wait for isRunning() == false.
   */
  void runEventTest(
      int poll_interval_ms, const EventCriteria &criteria,
      std::function<void(const std::vector<TestResult> &)> event_callback,
      long long max_duration_ms = 0);

  /**
   * @brief Run a specific subset of tests by name
   */
  std::vector<TestResult>
  runSelectedTests(const std::vector<std::string> &testNames);

  /**
   * @brief Return all available test names
   */
  virtual std::vector<std::string> getAvailableTests() = 0;

  /**
   * @brief Stop any active background test thread and join it
   */
  void stop();

  /**
   * @brief Non-blocking stop request — sets the stop flag without joining.
   * Safe to call from within a callback or the worker thread itself.
   */
  void requestStop();

  /**
   * @brief Pause execution of an active background test loop.
   * The current cycle completes; the next cycle is held until resume() is
   * called. Has no effect on SINGLE_SHOT / SINGLE_SHOT tests (they are already
   * synchronous).
   */
  virtual void pause();

  /**
   * @brief Resume a paused background test loop.
   */
  virtual void resume();

  /**
   * @brief Returns true while a background test thread is running
   */
  bool isRunning() const;

  /**
   * @brief Returns true if the test loop is paused (between cycles).
   */
  bool isPaused() const;

protected:
  virtual std::vector<TestResult> runAllTests() = 0;
  virtual TestResult runSingleTest(const std::string &testName) = 0;

private:
  // Exception-safe wrapper: any exception from runAllTests() is caught and
  // returned as a single ERROR TestResult instead of crashing the thread.
  std::vector<TestResult> runAllTestsSafe();

  // Fill in timestamp, sequence, and module_id on a TestResult.
  void prepareResult(TestResult &r);

  // Block the calling thread while paused_ is set (honours stopped_ to exit
  // early).
  void waitWhilePaused();

  // Sleep until wake_time or until stop() is called (whichever comes first).
  // Returns true if stopped_ was set during the wait.
  bool waitUntilOrStop(const std::chrono::steady_clock::time_point &wake_time);

  std::atomic<bool> stopped_;
  std::atomic<bool> paused_{false};
  std::thread monitoring_thread_;
  std::atomic<uint64_t> sequenceCounter_{0};
  std::mutex cv_mutex_;
  std::condition_variable cv_;
};

#endif // TEST_BASE_H