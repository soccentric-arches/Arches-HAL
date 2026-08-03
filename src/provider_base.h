/**
 * @file provider_base.h
 * @brief Abstract provider interfaces for hardware access injection
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 *
 * All HAL modules use these provider interfaces to decouple hardware access
 * from test logic.  Production code uses SystemProvider (real sysfs, I2C,
 * commands).  Tests use FakeProvider (deterministic, no hardware required).
 *
 * Every module that reads sysfs, runs commands, or accesses hardware should
 * accept a provider reference so native CI can run without physical hardware.
 */
#pragma once

#include "lib_base.h"

#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

// ============================================================
// SysfsProvider — read/write sysfs files
// ============================================================

class SysfsProvider {
public:
  virtual ~SysfsProvider() = default;

  /// Read a single line from a sysfs file.  Returns empty string on failure.
  virtual std::string readFile(const std::string &path) const = 0;

  /// Read an integer from a sysfs file.  Returns -1 on failure.
  virtual long long readInt(const std::string &path) const = 0;

  /// Write a value to a sysfs file.  Returns true on success.
  virtual bool writeFile(const std::string &path, const std::string &value) = 0;

  /// Check if a sysfs path exists.
  virtual bool pathExists(const std::string &path) const = 0;

  /// List directory entries matching a prefix.
  virtual std::vector<std::string> listDir(const std::string &path) const = 0;
};

// ============================================================
// CommandProvider — run shell commands
// ============================================================

class CommandProvider {
public:
  virtual ~CommandProvider() = default;

  /// Run a command and return its stdout.  Returns empty string on failure.
  virtual std::string runCommand(const std::string &cmd) const = 0;

  /// Run a command and return its exit status.  stdout is discarded.
  virtual int runCommandStatus(const std::string &cmd) const = 0;
};

// ============================================================
// ThermalZoneProvider — thermal zone abstraction
// ============================================================

struct ThermalZoneInfo {
  std::string name;
  double temperatureC{-1.0};
  bool valid{false};
};

class ThermalZoneProvider {
public:
  virtual ~ThermalZoneProvider() = default;

  /// Read all available thermal zones.
  virtual std::vector<ThermalZoneInfo> readAllZones() const = 0;

  /// Read a specific thermal zone by index.  Returns invalid info on failure.
  virtual ThermalZoneInfo readZone(int index) const = 0;

  /// Read trip points for a zone.
  struct TripPoint {
    std::string type;
    double temperatureC{-1.0};
  };
  virtual std::vector<TripPoint> readTripPoints(int zoneIndex) const = 0;

  /// Read cooling device state.
  struct CoolingDevice {
    std::string type;
    long long curState{0};
    long long maxState{0};
  };
  virtual std::vector<CoolingDevice> readCoolingDevices() const = 0;
};

// ============================================================
// Real implementations (production)
// ============================================================

class RealSysfsProvider : public SysfsProvider {
public:
  std::string readFile(const std::string &path) const override;
  long long readInt(const std::string &path) const override;
  bool writeFile(const std::string &path, const std::string &value) override;
  bool pathExists(const std::string &path) const override;
  std::vector<std::string> listDir(const std::string &path) const override;
};

class RealCommandProvider : public CommandProvider {
public:
  std::string runCommand(const std::string &cmd) const override;
  int runCommandStatus(const std::string &cmd) const override;
};

class RealThermalZoneProvider : public ThermalZoneProvider {
public:
  explicit RealThermalZoneProvider(
      std::shared_ptr<SysfsProvider> sysfs = nullptr,
      std::shared_ptr<CommandProvider> cmd = nullptr);

  std::vector<ThermalZoneInfo> readAllZones() const override;
  ThermalZoneInfo readZone(int index) const override;
  std::vector<TripPoint> readTripPoints(int zoneIndex) const override;
  std::vector<CoolingDevice> readCoolingDevices() const override;

private:
  std::shared_ptr<SysfsProvider> sysfs_;
  std::shared_ptr<CommandProvider> cmd_;
};

// ============================================================
// Fake implementations (testing)
// ============================================================

class FakeSysfsProvider : public SysfsProvider {
public:
  /// Set a file's contents.  Empty = file does not exist.
  void setFile(const std::string &path, const std::string &content);

  std::string readFile(const std::string &path) const override;
  long long readInt(const std::string &path) const override;
  bool writeFile(const std::string &path, const std::string &value) override;
  bool pathExists(const std::string &path) const override;
  std::vector<std::string> listDir(const std::string &path) const override;

private:
  std::map<std::string, std::string> files_;
};

class FakeCommandProvider : public CommandProvider {
public:
  /// Set a command's output.  Empty = command fails.
  void setOutput(const std::string &cmd, const std::string &output,
                 int exitCode = 0);

  std::string runCommand(const std::string &cmd) const override;
  int runCommandStatus(const std::string &cmd) const override;

private:
  struct CmdResult {
    std::string output;
    int exitCode{0};
  };
  std::map<std::string, CmdResult> commands_;
};

class FakeThermalZoneProvider : public ThermalZoneProvider {
public:
  void addZone(const std::string &name, double temperatureC);
  void addTripPoint(int zoneIndex, const std::string &type,
                    double temperatureC);
  void addCoolingDevice(const std::string &type, long long curState,
                        long long maxState);
  void clear();

  std::vector<ThermalZoneInfo> readAllZones() const override;
  ThermalZoneInfo readZone(int index) const override;
  std::vector<TripPoint> readTripPoints(int zoneIndex) const override;
  std::vector<CoolingDevice> readCoolingDevices() const override;

private:
  std::vector<ThermalZoneInfo> zones_;
  std::map<int, std::vector<TripPoint>> tripPoints_;
  std::vector<CoolingDevice> coolingDevices_;
};
