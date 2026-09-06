#pragma once
/**
 * @file src/instrumentation/hal_instrumentation.h
 * @brief Optional Arches-Instrumentation alarm reporting for HAL.
 *
 * R5 (Arches-Instrumentation REQ): an alarm is declared by an application and
 * by nothing else. When a hardware module faults, HAL previously wrote to the
 * journal and nothing else, so the condition was invisible to the local UI, to
 * the health score, and to HQ.
 *
 * OPTIONAL IN TWO SENSES, and both matter:
 *
 *   1. Compile time -- without the instrumentation package this compiles to
 *      no-ops (ARCHES_WITH_INSTRUMENTATION undefined). HAL is a library linked
 *      by Arches-Client and by 36 standalone test binaries; none of them may
 *      gain a hard dependency on instrumentation being installed.
 *
 *   2. Run time -- a missing or unreachable
 *      /run/instrumentation/application.sock is normal, not an error.
 *
 * Alarm ids are allocated to arches-hal in the application domain, module 0x10
 * (Arches-Instrumentation/catalog/alarms.yaml). See Task.md E-05.
 */

#include <cstdint>
#include <string>

namespace arches::hal::instrumentation {

/// Owner on every alarm call. Required by R5, and identical on raise and clear.
inline constexpr const char* kOwner = "arches-hal";

/// Alarm ids for arches-hal (catalog module 0x10).
enum AlarmId : uint32_t {
    kModuleFault = 0x41010001u,       ///< hal_module_fault
    kModuleUnavailable = 0x41010002u, ///< hal_module_unavailable
    kStm32LinkLost = 0x41020003u,     ///< hal_stm32_link_lost
};

/// @brief Raise an alarm. No-op when instrumentation is absent or unreachable.
/// @param id One of AlarmId.
/// @param detail Free text appended as additional_text.
void raise(AlarmId id, const std::string& detail);

/// @brief Clear an alarm previously raised with the same id.
void clear(AlarmId id);

} // namespace arches::hal::instrumentation
