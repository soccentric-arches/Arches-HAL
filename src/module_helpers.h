/**
 * @file module_helpers.h
 * @brief Shared helper utilities for all HAL modules
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * Provides common patterns used across all 35 modules:
 * - Typed metric helpers
 * - SKIPPED/UNAVAILABLE result builders
 * - Periodic-safe test classification
 * - Common sysfs/command reading patterns
 */
#pragma once

#include "lib_base.h"
#include "provider_base.h"

#include <cctype>
#include <memory>
#include <string>
#include <vector>

// ============================================================
// Result builders
// ============================================================

/// Build a SKIPPED TestOutput for absent hardware.
inline TestOutput makeSkipped(const std::string& reason) {
    TestOutput out;
    out.status = hal::Status::SKIPPED;
    out.severity = hal::Severity::INFO;
    out.reason["detail"] = reason;
    return out;
}

/// Build a FAIL TestOutput with an error message.
inline TestOutput makeFailed(const std::string& error, hal::Severity sev = hal::Severity::ERROR) {
    TestOutput out;
    out.status = hal::Status::FAIL;
    out.severity = sev;
    out.reason["error"] = error;
    return out;
}

/// Build a PASS TestOutput.
inline TestOutput makePassed(const std::string& message = "") {
    TestOutput out;
    out.status = hal::Status::PASS;
    out.severity = hal::Severity::INFO;
    if (!message.empty())
        out.reason["result"] = message;
    return out;
}

/// Add a typed metric to a TestOutput.
inline void addMetric(TestOutput& out, const std::string& name, const hal::Value& value) {
    out.metrics[name] = value;
}

/// Add a monitoring value to a TestOutput.
inline void addMonitor(TestOutput& out, const std::string& name, const hal::Value& value) {
    out.monitoring[name] = value;
}

/// Add a reason detail to a TestOutput.
inline void addDetail(TestOutput& out, const std::string& key, const std::string& value) {
    out.reason[key] = value;
}

// ============================================================
// Numeric parsing helpers (exception-safe)
// ============================================================

/// Parse a double from s; returns fallback on empty/invalid input.
/// Use instead of bare std::stod on sysfs/command output so a malformed
/// value becomes a graceful skip/default rather than a thrown exception.
inline double parseDoubleOr(const std::string& s, double fallback) {
    try {
        size_t pos = 0;
        double v = std::stod(s, &pos);
        if (pos != s.size())
            return fallback;
        return v;
    } catch (...) {
        return fallback;
    }
}

/// Parse an integer from s; returns fallback on empty/invalid input.
inline long long parseIntOr(const std::string& s, long long fallback) {
    try {
        size_t pos = 0;
        long long v = std::stoll(s, &pos);
        if (pos != s.size())
            return fallback;
        return v;
    } catch (...) {
        return fallback;
    }
}

// ============================================================
// Periodic-safe test classification
// ============================================================

/// Run only the periodic-safe subset of tests.
/// Override in each module to separate passive health checks from
/// destructive/state-changing tests.
inline std::vector<std::string> filterPeriodicSafe(const std::vector<std::string>& allTests,
                                                   const std::vector<std::string>& oneShotOnly) {
    std::vector<std::string> safe;
    for (const auto& t : allTests) {
        bool isOneShot = false;
        for (const auto& os : oneShotOnly) {
            if (t == os) {
                isOneShot = true;
                break;
            }
        }
        if (!isOneShot)
            safe.push_back(t);
    }
    return safe;
}

// ============================================================
// Common sysfs reading helpers (use injected providers)
// ============================================================

/// True when name is safe to interpolate into a shell command:
/// alphanumeric, underscore, hyphen, dot, colon only, non-empty,
/// no leading hyphen (so it cannot be parsed as a command flag).
/// Sysfs/devfs entry names come from the kernel and match this, but
/// providers are injectable — validate before building command strings.
inline bool isSafeName(const std::string& name) {
    if (name.empty() || name[0] == '-')
        return false;
    for (const char c : name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.' ||
              c == ':'))
            return false;
    }
    return true;
}

/// Read a sysfs file and return its trimmed content.
inline std::string readSysfs(const std::shared_ptr<SysfsProvider>& sysfs, const std::string& path) {
    return sysfs ? sysfs->readFile(path) : "";
}

/// Read a sysfs integer value.
inline long long readSysfsInt(const std::shared_ptr<SysfsProvider>& sysfs,
                              const std::string& path) {
    return sysfs ? sysfs->readInt(path) : -1LL;
}

/// Read the first existing file from a list of paths.
inline std::string readFirstExisting(const std::shared_ptr<SysfsProvider>& sysfs,
                                     const std::vector<std::string>& paths) {
    if (!sysfs)
        return "";
    for (const auto& p : paths) {
        std::string v = sysfs->readFile(p);
        if (!v.empty())
            return v;
    }
    return "";
}

/// Run a command and return its output.
inline std::string runCommand(const std::shared_ptr<CommandProvider>& cmd,
                              const std::string& command) {
    return cmd ? cmd->runCommand(command) : "";
}
