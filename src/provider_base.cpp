/**
 * @file provider_base.cpp
 * @brief Implementation of provider interfaces
 * @author sandesh ghimire
 * @copyright (C) Soccentric LLC
 */
#include "provider_base.h"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>

// ============================================================
// RealSysfsProvider
// ============================================================

std::string RealSysfsProvider::readFile(const std::string& path) const {
    std::ifstream f(path);
    if (!f.is_open())
        return "";
    std::string s;
    std::getline(f, s);
    // Strip trailing whitespace
    while (!s.empty() &&
           (s.back() == '\n' || s.back() == '\r' || s.back() == ' ' || s.back() == '\t')) {
        s.pop_back();
    }
    return s;
}

long long RealSysfsProvider::readInt(const std::string& path) const {
    std::string s = readFile(path);
    if (s.empty())
        return -1LL;
    try {
        return std::stoll(s);
    } catch (...) {
        return -1LL;
    }
}

bool RealSysfsProvider::writeFile(const std::string& path, const std::string& value) {
    std::ofstream f(path);
    if (!f.is_open())
        return false;
    f << value;
    return !f.fail();
}

bool RealSysfsProvider::pathExists(const std::string& path) const {
    std::ifstream f(path);
    return f.is_open();
}

std::vector<std::string> RealSysfsProvider::listDir(const std::string& path) const {
    std::vector<std::string> entries;
    DIR* dir = opendir(path.c_str());
    if (!dir)
        return entries;
    struct dirent* e;
    while ((e = readdir(dir)) != nullptr) {
        if (e->d_name[0] == '.')
            continue;
        entries.emplace_back(e->d_name);
    }
    closedir(dir);
    std::sort(entries.begin(), entries.end());
    return entries;
}

// ============================================================
// RealCommandProvider
// ============================================================

std::string RealCommandProvider::runCommand(const std::string& cmd) const {
    char buf[4096] = {};
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";
    size_t n = fread(buf, 1, sizeof(buf) - 1, pipe);
    int rc = pclose(pipe);
    (void)rc;
    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
        buf[--n] = '\0';
    return std::string(buf);
}

int RealCommandProvider::runCommandStatus(const std::string& cmd) const {
    return system(cmd.c_str());
}

// ============================================================
// RealThermalZoneProvider
// ============================================================

RealThermalZoneProvider::RealThermalZoneProvider(std::shared_ptr<SysfsProvider> sysfs,
                                                 std::shared_ptr<CommandProvider> cmd)
    : sysfs_(sysfs ? sysfs : std::make_shared<RealSysfsProvider>()),
      cmd_(cmd ? cmd : std::make_shared<RealCommandProvider>()) {}

std::vector<ThermalZoneInfo> RealThermalZoneProvider::readAllZones() const {
    std::vector<ThermalZoneInfo> zones;
    for (int z = 0; z < 20; ++z) {
        std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(z);
        std::string type = sysfs_->readFile(base + "/type");
        if (type.empty())
            break;
        long long milliC = sysfs_->readInt(base + "/temp");
        if (milliC < -50000 || milliC > 200000)
            continue;
        ThermalZoneInfo info;
        info.name = type;
        info.temperatureC = milliC / 1000.0;
        info.valid = true;
        zones.push_back(info);
    }
    return zones;
}

ThermalZoneInfo RealThermalZoneProvider::readZone(int index) const {
    std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(index);
    std::string type = sysfs_->readFile(base + "/type");
    if (type.empty())
        return {};
    long long milliC = sysfs_->readInt(base + "/temp");
    if (milliC < -50000 || milliC > 200000)
        return {};
    ThermalZoneInfo info;
    info.name = type;
    info.temperatureC = milliC / 1000.0;
    info.valid = true;
    return info;
}

std::vector<RealThermalZoneProvider::TripPoint>
RealThermalZoneProvider::readTripPoints(int zoneIndex) const {
    std::vector<TripPoint> trips;
    std::string base = "/sys/class/thermal/thermal_zone" + std::to_string(zoneIndex);
    for (int t = 0; t < 10; ++t) {
        std::string tbase = base + "/trip_point_" + std::to_string(t);
        std::string type = sysfs_->readFile(tbase + "_type");
        if (type.empty())
            break;
        long long temp = sysfs_->readInt(tbase + "_temp");
        TripPoint tp;
        tp.type = type;
        tp.temperatureC = (temp > 0) ? (temp / 1000.0) : -1.0;
        trips.push_back(tp);
    }
    return trips;
}

std::vector<RealThermalZoneProvider::CoolingDevice>
RealThermalZoneProvider::readCoolingDevices() const {
    std::vector<CoolingDevice> devices;
    for (int c = 0; c < 8; ++c) {
        std::string base = "/sys/class/thermal/cooling_device" + std::to_string(c);
        std::string type = sysfs_->readFile(base + "/type");
        if (type.empty())
            break;
        CoolingDevice cd;
        cd.type = type;
        cd.curState = sysfs_->readInt(base + "/cur_state");
        cd.maxState = sysfs_->readInt(base + "/max_state");
        if (cd.curState < 0)
            cd.curState = 0;
        if (cd.maxState < 0)
            cd.maxState = 0;
        devices.push_back(cd);
    }
    return devices;
}

// ============================================================
// FakeSysfsProvider
// ============================================================

void FakeSysfsProvider::setFile(const std::string& path, const std::string& content) {
    if (content.empty()) {
        files_.erase(path);
    } else {
        files_[path] = content;
    }
}

std::string FakeSysfsProvider::readFile(const std::string& path) const {
    auto it = files_.find(path);
    if (it == files_.end())
        return "";
    return it->second;
}

long long FakeSysfsProvider::readInt(const std::string& path) const {
    std::string s = readFile(path);
    if (s.empty())
        return -1LL;
    try {
        return std::stoll(s);
    } catch (...) {
        return -1LL;
    }
}

bool FakeSysfsProvider::writeFile(const std::string& path, const std::string& value) {
    files_[path] = value;
    return true;
}

bool FakeSysfsProvider::pathExists(const std::string& path) const {
    return files_.find(path) != files_.end();
}

std::vector<std::string> FakeSysfsProvider::listDir(const std::string& path) const {
    std::vector<std::string> entries;
    for (const auto& kv : files_) {
        if (kv.first.rfind(path, 0) == 0) {
            std::string rest = kv.first.substr(path.size());
            // Skip leading '/'
            if (!rest.empty() && rest[0] == '/')
                rest = rest.substr(1);
            auto slash = rest.find('/');
            std::string entry = (slash != std::string::npos) ? rest.substr(0, slash) : rest;
            if (!entry.empty() && entry[0] != '.' &&
                std::find(entries.begin(), entries.end(), entry) == entries.end()) {
                entries.push_back(entry);
            }
        }
    }
    std::sort(entries.begin(), entries.end());
    return entries;
}

// ============================================================
// FakeCommandProvider
// ============================================================

void FakeCommandProvider::setOutput(const std::string& cmd, const std::string& output,
                                    int exitCode) {
    commands_[cmd] = {output, exitCode};
}

std::string FakeCommandProvider::runCommand(const std::string& cmd) const {
    auto it = commands_.find(cmd);
    if (it == commands_.end())
        return "";
    return it->second.output;
}

int FakeCommandProvider::runCommandStatus(const std::string& cmd) const {
    auto it = commands_.find(cmd);
    if (it == commands_.end())
        return -1;
    return it->second.exitCode;
}

// ============================================================
// FakeThermalZoneProvider
// ============================================================

void FakeThermalZoneProvider::addZone(const std::string& name, double temperatureC) {
    ThermalZoneInfo info;
    info.name = name;
    info.temperatureC = temperatureC;
    info.valid = true;
    zones_.push_back(info);
}

void FakeThermalZoneProvider::addTripPoint(int zoneIndex, const std::string& type,
                                           double temperatureC) {
    TripPoint tp;
    tp.type = type;
    tp.temperatureC = temperatureC;
    tripPoints_[zoneIndex].push_back(tp);
}

void FakeThermalZoneProvider::addCoolingDevice(const std::string& type, long long curState,
                                               long long maxState) {
    CoolingDevice cd;
    cd.type = type;
    cd.curState = curState;
    cd.maxState = maxState;
    coolingDevices_.push_back(cd);
}

void FakeThermalZoneProvider::clear() {
    zones_.clear();
    tripPoints_.clear();
    coolingDevices_.clear();
}

std::vector<ThermalZoneInfo> FakeThermalZoneProvider::readAllZones() const { return zones_; }

ThermalZoneInfo FakeThermalZoneProvider::readZone(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= zones_.size())
        return {};
    return zones_[index];
}

std::vector<FakeThermalZoneProvider::TripPoint>
FakeThermalZoneProvider::readTripPoints(int zoneIndex) const {
    auto it = tripPoints_.find(zoneIndex);
    if (it == tripPoints_.end())
        return {};
    return it->second;
}

std::vector<FakeThermalZoneProvider::CoolingDevice>
FakeThermalZoneProvider::readCoolingDevices() const {
    return coolingDevices_;
}
