/**
 * @file rtc_lib.cpp
 * @brief Implementation file for RtcTesting class
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC */

#include "rtc_lib.h"

#include <chrono>
#include <cmath>
#include <ctime>
#include <vector>

namespace {

struct TestEntry {
  const char *name;
  TestOutput (RtcTesting::*fn)();
};

const std::string kRtcDir = "/sys/class/rtc/rtc0";

/// Parse "HH:MM:SS" into seconds since midnight. Returns -1 on failure.
long long parseHms(const std::string &s) {
  if (s.size() != 8 || s[2] != ':' || s[5] != ':')
    return -1;
  try {
    const int h = std::stoi(s.substr(0, 2));
    const int m = std::stoi(s.substr(3, 2));
    const int sec = std::stoi(s.substr(6, 2));
    if (h < 0 || h > 23 || m < 0 || m > 59 || sec < 0 || sec > 60)
      return -1;
    return h * 3600LL + m * 60LL + sec;
  } catch (...) {
    return -1;
  }
}

/// Parse "YYYY-MM-DD" into days since epoch (proleptic Gregorian).
/// Returns -1 on failure.
long long parseYmdDays(const std::string &s) {
  if (s.size() != 10 || s[4] != '-' || s[7] != '-')
    return -1;
  try {
    const long long y = std::stoll(s.substr(0, 4));
    const long long m = std::stoll(s.substr(5, 2));
    const long long d = std::stoll(s.substr(8, 2));
    if (y < 1970 || m < 1 || m > 12 || d < 1 || d > 31)
      return -1;
    // Howard Hinnant's civil_from_days inverse (days_from_civil).
    const long long yy = y - (m <= 2);
    const long long era = (yy >= 0 ? yy : yy - 399) / 400;
    const long long yoe = yy - era * 400;
    const long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097LL + doe - 719468LL;
  } catch (...) {
    return -1;
  }
}

} // anonymous namespace

RtcTesting::RtcTesting()
    : sysfs_(std::make_shared<RealSysfsProvider>()),
      cmd_(std::make_shared<RealCommandProvider>()) {}

RtcTesting::~RtcTesting() {}

RtcTesting::RtcTesting(std::shared_ptr<SysfsProvider> sysfs,
                       std::shared_ptr<CommandProvider> cmd)
    : sysfs_(std::move(sysfs)),
      cmd_(cmd ? std::move(cmd) : std::make_shared<RealCommandProvider>()) {}

TestOutput RtcTesting::testRtcDevicePresence() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  const auto entries = sysfs_->listDir("/sys/class/rtc");
  if (entries.empty())
    return makeSkipped("no rtc class device present");
  TestOutput out = makePassed("RTC device present");
  addMetric(out, "rtc_count", static_cast<int64_t>(entries.size()));
  return out;
}

TestOutput RtcTesting::testRtcTimeRead() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  if (sysfs_->listDir("/sys/class/rtc").empty())
    return makeSkipped("no rtc class device present");
  const std::string time = readSysfs(sysfs_, kRtcDir + "/time");
  const std::string date = readSysfs(sysfs_, kRtcDir + "/date");
  const long long secs = parseHms(time);
  const long long days = parseYmdDays(date);
  if (secs < 0 || days < 0)
    return makeFailed("RTC time/date unreadable or malformed (time='" + time +
                      "' date='" + date + "')");
  TestOutput out = makePassed("RTC time and date read");
  addMetric(out, "time_of_day_s", secs);
  addDetail(out, "time", time);
  addDetail(out, "date", date);
  return out;
}

TestOutput RtcTesting::testHctosys() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  if (sysfs_->listDir("/sys/class/rtc").empty())
    return makeSkipped("no rtc class device present");
  const std::string v = readSysfs(sysfs_, kRtcDir + "/hctosys");
  if (v != "0" && v != "1")
    return makeSkipped("hctosys attribute unreadable");
  TestOutput out =
      makePassed(v == "1" ? "RTC was used to set the system clock"
                          : "RTC not used to set the system clock");
  addMetric(out, "hctosys", v == "1");
  return out;
}

TestOutput RtcTesting::testAlarmCapability() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  if (sysfs_->listDir("/sys/class/rtc").empty())
    return makeSkipped("no rtc class device present");
  const auto entries = sysfs_->listDir(kRtcDir);
  bool hasWakealarm = false;
  for (const auto &e : entries) {
    if (e == "wakealarm")
      hasWakealarm = true;
  }
  if (!hasWakealarm)
    return makeSkipped("RTC does not expose wakealarm (no alarm support)");
  TestOutput out = makePassed("RTC alarm capability present");
  addMetric(out, "alarm_capable", true);
  return out;
}

TestOutput RtcTesting::testClockSkew() {
  if (!sysfs_)
    return makeSkipped("no sysfs provider");
  if (sysfs_->listDir("/sys/class/rtc").empty())
    return makeSkipped("no rtc class device present");
  const long long rtcSecs = parseHms(readSysfs(sysfs_, kRtcDir + "/time"));
  const long long rtcDays = parseYmdDays(readSysfs(sysfs_, kRtcDir + "/date"));
  if (rtcSecs < 0 || rtcDays < 0)
    return makeSkipped("RTC time/date unreadable; cannot compute skew");

  const std::time_t now = std::time(nullptr);
  if (now == static_cast<std::time_t>(-1))
    return makeSkipped("system time unavailable");
  // System clock seconds since epoch (UTC); RTC sysfs date/time are also UTC.
  const long long sysEpoch = static_cast<long long>(now);
  const long long sysDays = sysEpoch / 86400LL;
  const long long sysSecs = sysEpoch % 86400LL;
  long long skew =
      (rtcDays * 86400LL + rtcSecs) - (sysDays * 86400LL + sysSecs);
  skew = std::llabs(skew);
  // RTC may be in local time; fold skew into [-12h, +12h] window.
  if (skew > 43200LL)
    skew = 86400LL - skew;
  TestOutput out = makePassed(skew <= 60 ? "RTC within 60s of system clock"
                                         : "RTC skew exceeds 60s");
  if (skew > 60) {
    out.status = hal::Status::FAIL;
    out.severity = hal::Severity::WARNING;
  }
  addMetric(out, "skew_s", static_cast<double>(skew));
  return out;
}

std::vector<std::string> RtcTesting::getAvailableTests() {
  return {"testRtcDevicePresence", "testRtcTimeRead", "testHctosys",
          "testAlarmCapability", "testClockSkew"};
}

std::vector<std::string> RtcTesting::getPeriodicSafeTests() {
  return {"testRtcDevicePresence", "testRtcTimeRead", "testHctosys",
          "testAlarmCapability", "testClockSkew"};
}

std::vector<std::string> RtcTesting::getOneShotOnlyTests() { return {}; }

std::vector<TestResult> RtcTesting::runAllTests() {
  std::vector<TestResult> results;
  for (const auto &name : getAvailableTests())
    results.push_back(runSingleTest(name));
  return results;
}

TestResult RtcTesting::runSingleTest(const std::string &testName) {
  auto start = std::chrono::high_resolution_clock::now();
  TestOutput out;
  out.status = hal::Status::ERROR;
  out.severity = hal::Severity::ERROR;
  out.error_message = "Unknown test name: " + testName;

  const std::vector<TestEntry> table = {
      {"testRtcDevicePresence", &RtcTesting::testRtcDevicePresence},
      {"testRtcTimeRead", &RtcTesting::testRtcTimeRead},
      {"testHctosys", &RtcTesting::testHctosys},
      {"testAlarmCapability", &RtcTesting::testAlarmCapability},
      {"testClockSkew", &RtcTesting::testClockSkew},
  };
  for (const auto &e : table) {
    if (testName == e.name) {
      out = (this->*e.fn)();
      break;
    }
  }

  TestResult r;
  r.name = testName;
  r.test_id = testName;
  r.output = out;
  r.duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::high_resolution_clock::now() - start);
  return r;
}