/**
 * @file lib_base.cpp
 * @brief Implementation of TestBase class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "lib_base.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

TestBase::TestBase() : stopped_(true) {}

TestBase::~TestBase() { stop(); }

// ---------------------------------------------------------------------------
// Internal: exception-safe wrapper around runAllTests().
// Uses runSelectedTests() to isolate exceptions per test so one failing
// test does not suppress the remaining tests in a cycle.
// Returns a single ERROR result if any exception escapes the derived class.
// ---------------------------------------------------------------------------
std::vector<TestResult> TestBase::runAllTestsSafe() {
    try {
        auto results = runSelectedTests(getAvailableTests());
        for (auto& r : results) {
            prepareResult(r);
        }
        return results;
    } catch (const std::exception& ex) {
        TestResult err;
        err.name = "runAllTests_exception";
        err.test_id = err.name;
        err.output.status = hal::Status::ERROR;
        err.output.severity = hal::Severity::CRITICAL;
        err.output.error_message = std::string("Exception in runAllTests: ") + ex.what();
        prepareResult(err);
        std::cerr << "[TestBase] " << err.output.error_message << "\n";
        return {err};
    } catch (...) {
        TestResult err;
        err.name = "runAllTests_exception";
        err.test_id = err.name;
        err.output.status = hal::Status::ERROR;
        err.output.severity = hal::Severity::CRITICAL;
        err.output.error_message = "Unknown exception in runAllTests";
        prepareResult(err);
        std::cerr << "[TestBase] " << err.output.error_message << "\n";
        return {err};
    }
}

// Internal: fill in timestamp, sequence, and module_id on a TestResult.
void TestBase::prepareResult(TestResult& r) {
    if (r.timestamp.time_since_epoch().count() == 0) {
        r.timestamp = std::chrono::system_clock::now();
    }
    if (r.sequence == 0) {
        r.sequence = ++sequenceCounter_;
    }
    if (r.module_id.empty()) {
        r.module_id = "Arches-HAL";
    }
    if (r.test_id.empty()) {
        r.test_id = r.name;
    }
}

// Internal: wait while paused (checked at the start of each background cycle).
void TestBase::waitWhilePaused() {
    while (paused_.load() && !stopped_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

// Internal: sleep until wake_time or until stop() is called.
// Returns true if stopped_ was set during the wait.
bool TestBase::waitUntilOrStop(const std::chrono::steady_clock::time_point& wake_time) {
    std::unique_lock<std::mutex> lock(cv_mutex_);
    // wait_until returns cv_status::timeout if wake_time elapsed,
    // cv_status::no_timeout if notified (stop).
    cv_.wait_until(lock, wake_time, [this]() { return stopped_.load(); });
    return stopped_.load();
}

// ---------------------------------------------------------------------------
// SINGLE_SHOT / MONITORING (backward-compatible)
// ---------------------------------------------------------------------------

void TestBase::runTests(TestMode mode, std::function<void(const std::vector<TestResult>&)> callback,
                        int interval_ms) {
    if (mode == TestMode::SINGLE_SHOT) {
        auto results = runAllTestsSafe();
        if (callback) {
            callback(results);
        }
    } else if (mode == TestMode::MONITORING) {
        if (interval_ms <= 0) {
            std::cerr << "[TestBase] ERROR: interval_ms must be positive, got " << interval_ms
                      << "\n";
            return;
        }
        stop();
        stopped_ = false;
        paused_ = false;
        monitoring_thread_ = std::thread([this, callback, interval_ms]() {
            while (!stopped_) {
                waitWhilePaused();
                if (stopped_)
                    break;
                auto results = runAllTestsSafe();
                if (callback) {
                    try {
                        callback(results);
                    } catch (const std::exception& ex) {
                        std::cerr << "[TestBase] Callback exception in MONITORING: " << ex.what()
                                  << "\n";
                    } catch (...) {
                        std::cerr << "[TestBase] Unknown callback exception in MONITORING\n";
                    }
                }
                if (waitUntilOrStop(std::chrono::steady_clock::now() +
                                    std::chrono::milliseconds(interval_ms)))
                    break;
            }
        });
    } else if (mode == TestMode::LONG_TERM || mode == TestMode::EVENT_BASED) {
        // These modes have dedicated entry points; route loudly instead of
        // silently doing nothing.
        std::cerr << "[TestBase] ERROR: runTests() does not support mode " << static_cast<int>(mode)
                  << "; use runLongTerm() or runEventTest()\n";
    }
}

// ---------------------------------------------------------------------------
// LONG_TERM — fixed-duration periodic test
// ---------------------------------------------------------------------------

void TestBase::runLongTerm(int interval_ms, long long duration_ms,
                           std::function<void(const std::vector<TestResult>&, int)> callback) {
    if (interval_ms <= 0) {
        std::cerr << "[TestBase] ERROR: interval_ms must be positive, got " << interval_ms << "\n";
        return;
    }
    if (duration_ms < 0) {
        std::cerr << "[TestBase] ERROR: duration_ms must be non-negative, got " << duration_ms
                  << "\n";
        return;
    }
    stop();
    stopped_ = false;
    paused_ = false;
    monitoring_thread_ = std::thread([this, interval_ms, duration_ms, callback]() {
        auto start = std::chrono::steady_clock::now();
        int cycle = 0;
        while (!stopped_) {
            waitWhilePaused();
            if (stopped_)
                break;
            if (duration_ms > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - start)
                                   .count();
                if (elapsed >= duration_ms) {
                    break;
                }
            }
            auto results = runAllTestsSafe();
            ++cycle;
            if (callback) {
                try {
                    callback(results, cycle);
                } catch (const std::exception& ex) {
                    std::cerr << "[TestBase] Callback exception in LONG_TERM: " << ex.what()
                              << "\n";
                } catch (...) {
                    std::cerr << "[TestBase] Unknown callback exception in LONG_TERM\n";
                }
            }
            if (waitUntilOrStop(std::chrono::steady_clock::now() +
                                std::chrono::milliseconds(interval_ms)))
                break;
        }
        stopped_ = true;
    });
}

// ---------------------------------------------------------------------------
// MONITORING TEST — long-term + hardware monitor sampling
// ---------------------------------------------------------------------------

void TestBase::runMonitoringTest(int interval_ms, long long duration_ms,
                                 const std::vector<MonitorItem>& monitorItems,
                                 std::function<void(const MonitoringSnapshot&)> callback) {
    if (interval_ms <= 0) {
        std::cerr << "[TestBase] ERROR: interval_ms must be positive, got " << interval_ms << "\n";
        return;
    }
    if (duration_ms < 0) {
        std::cerr << "[TestBase] ERROR: duration_ms must be non-negative, got " << duration_ms
                  << "\n";
        return;
    }
    stop();
    stopped_ = false;
    paused_ = false;
    monitoring_thread_ = std::thread([this, interval_ms, duration_ms, monitorItems, callback]() {
        auto start = std::chrono::steady_clock::now();
        int cycle = 0;
        while (!stopped_) {
            waitWhilePaused();
            if (stopped_)
                break;
            if (duration_ms > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - start)
                                   .count();
                if (elapsed >= duration_ms) {
                    break;
                }
            }
            MonitoringSnapshot snap;
            snap.cycle = ++cycle;
            snap.timestamp = std::chrono::system_clock::now();
            snap.testResults = runAllTestsSafe();
            for (const auto& item : monitorItems) {
                MonitorValue mv;
                try {
                    mv = item.reader();
                } catch (const std::exception& ex) {
                    mv.availability = MonitorAvailability::ERROR;
                    mv.error_message = ex.what();
                } catch (...) {
                    mv.availability = MonitorAvailability::ERROR;
                    mv.error_message = "Unknown exception in monitor reader";
                }
                snap.monitorData.emplace_back(item.name, mv);
            }
            if (callback) {
                try {
                    callback(snap);
                } catch (const std::exception& ex) {
                    std::cerr << "[TestBase] Callback exception in MONITOR_TEST: " << ex.what()
                              << "\n";
                } catch (...) {
                    std::cerr << "[TestBase] Unknown callback exception in MONITOR_TEST\n";
                }
            }
            if (waitUntilOrStop(std::chrono::steady_clock::now() +
                                std::chrono::milliseconds(interval_ms)))
                break;
        }
        stopped_ = true;
    });
}

// ---------------------------------------------------------------------------
// EVENT TEST — fire callback when predicate is satisfied
// ---------------------------------------------------------------------------

void TestBase::runEventTest(int poll_interval_ms, const EventCriteria& criteria,
                            std::function<void(const std::vector<TestResult>&)> event_callback,
                            long long max_duration_ms) {
    if (poll_interval_ms <= 0) {
        std::cerr << "[TestBase] ERROR: poll_interval_ms must be positive, got " << poll_interval_ms
                  << "\n";
        return;
    }
    if (max_duration_ms < 0) {
        std::cerr << "[TestBase] ERROR: max_duration_ms must be non-negative, got "
                  << max_duration_ms << "\n";
        return;
    }
    stop();
    stopped_ = false;
    paused_ = false;
    monitoring_thread_ =
        std::thread([this, poll_interval_ms, max_duration_ms, criteria, event_callback]() {
            auto start = std::chrono::steady_clock::now();
            bool prev_fired = false;
            while (!stopped_) {
                waitWhilePaused();
                if (stopped_)
                    break;
                if (max_duration_ms > 0) {
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::steady_clock::now() - start)
                                       .count();
                    if (elapsed >= max_duration_ms) {
                        break;
                    }
                }
                auto results = runAllTestsSafe();
                bool now_fired = criteria.predicate && criteria.predicate(results);
                if (now_fired && !prev_fired) {
                    if (event_callback) {
                        try {
                            event_callback(results);
                        } catch (const std::exception& ex) {
                            std::cerr << "[TestBase] Callback exception in EVENT: " << ex.what()
                                      << "\n";
                        } catch (...) {
                            std::cerr << "[TestBase] Unknown callback exception in EVENT\n";
                        }
                    }
                }
                prev_fired = now_fired;
                if (waitUntilOrStop(std::chrono::steady_clock::now() +
                                    std::chrono::milliseconds(poll_interval_ms)))
                    break;
            }
            stopped_ = true;
        });
}

// ---------------------------------------------------------------------------
// Control
// ---------------------------------------------------------------------------

void TestBase::requestStop() {
    stopped_ = true;
    cv_.notify_all();
}

void TestBase::stop() {
    stopped_ = true;
    cv_.notify_all();
    // If the calling thread is the worker thread itself, do not join
    // (joining self is undefined behavior / deadlock).
    if (monitoring_thread_.joinable() &&
        monitoring_thread_.get_id() != std::this_thread::get_id()) {
        monitoring_thread_.join();
    }
}

bool TestBase::isRunning() const { return !stopped_; }

void TestBase::pause() { paused_.store(true); }

void TestBase::resume() { paused_.store(false); }

bool TestBase::isPaused() const { return paused_.load(); }

std::vector<TestResult> TestBase::runSelectedTests(const std::vector<std::string>& testNames) {
    const auto available = getAvailableTests();
    std::vector<TestResult> results;
    results.reserve(testNames.size());
    for (const auto& name : testNames) {
        bool found = false;
        for (const auto& avail : available) {
            if (avail == name) {
                found = true;
                break;
            }
        }
        if (!found) {
            TestResult err;
            err.name = name;
            err.test_id = name;
            err.output.status = hal::Status::ERROR;
            err.output.severity = hal::Severity::ERROR;
            err.output.error_message = "Unknown test name: \"" + name + "\"";
            prepareResult(err);
            results.push_back(std::move(err));
            continue;
        }
        try {
            auto r = runSingleTest(name);
            prepareResult(r);
            results.push_back(std::move(r));
        } catch (const std::exception& ex) {
            TestResult err;
            err.name = name;
            err.test_id = name;
            err.output.status = hal::Status::ERROR;
            err.output.severity = hal::Severity::CRITICAL;
            err.output.error_message =
                std::string("Exception in runSingleTest(\"") + name + "\"): " + ex.what();
            prepareResult(err);
            results.push_back(std::move(err));
        } catch (...) {
            TestResult err;
            err.name = name;
            err.test_id = name;
            err.output.status = hal::Status::ERROR;
            err.output.severity = hal::Severity::CRITICAL;
            err.output.error_message = "Unknown exception in runSingleTest(\"" + name + "\")";
            prepareResult(err);
            results.push_back(std::move(err));
        }
    }
    return results;
}