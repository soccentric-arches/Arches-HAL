/**
 * @file src/instrumentation/hal_instrumentation.cpp
 * @brief Optional Arches-Instrumentation alarm reporting (see the header).
 *
 * A free-function facade over a lazily-created connection, rather than an
 * object the caller owns. HAL's fault path is deep inside TestBase and has no
 * natural place to hold a reporter, and threading one through every module
 * would be a large change to satisfy an optional feature.
 */

#include "instrumentation/hal_instrumentation.h"

#include <iostream>

#ifdef ARCHES_WITH_INSTRUMENTATION
#include <chrono>
#include <memory>
#include <mutex>

#include <grpcpp/grpcpp.h>

#include "istate.grpc.pb.h"
#endif

namespace arches::hal::instrumentation {

namespace {
constexpr const char* kSocket = "/run/instrumentation/application.sock";
} // namespace

#ifdef ARCHES_WITH_INSTRUMENTATION

namespace {

/// Connect once, on first use. HAL is a library: there is no startup hook to
/// hang this off, and a module fault is rare enough that a lazy connect costs
/// nothing.
///
/// call_once rather than a plain static: modules run on their own monitoring
/// threads, so two can fault concurrently.
instr::istate::ApplicationApi::Stub* stub() {
    static std::once_flag once;
    static std::unique_ptr<instr::istate::ApplicationApi::Stub> s;
    std::call_once(once, [] {
        auto channel =
            grpc::CreateChannel(std::string("unix:") + kSocket, grpc::InsecureChannelCredentials());
        const auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(2);
        if (!channel->WaitForConnected(deadline)) {
            std::cerr << "[hal] instrumentation not available at " << kSocket
                      << " -- continuing without it\n";
            return;
        }
        s = instr::istate::ApplicationApi::NewStub(channel);
    });
    return s.get();
}

} // namespace

void raise(AlarmId id, const std::string& detail) {
    auto* st = stub();
    if (!st)
        return;
    instr::istate::RaiseAlarmRequest req;
    req.set_alarm_id(static_cast<uint32_t>(id));
    req.set_owner(kOwner); // required (R5)
    req.set_additional_text(detail);
    grpc::ClientContext ctx;
    instr::istate::AlarmAck ack;
    const grpc::Status s = st->RaiseAlarm(&ctx, req, &ack);
    if (!s.ok()) {
        std::cerr << "[hal] RaiseAlarm failed: " << s.error_message() << "\n";
        return;
    }
    if (ack.needs_snapshot()) {
        // A CRITICAL alarm asked for bounded diagnostics. HAL's own TestResult
        // already carries the error message and module id for this fault, and
        // that is what a snapshot would capture, so record the request rather
        // than duplicating the capture.
        std::cerr << "[hal] alarm 0x" << std::hex << static_cast<uint32_t>(id) << std::dec
                  << " requested a diagnostic snapshot\n";
    }
}

void clear(AlarmId id) {
    auto* st = stub();
    if (!st)
        return;
    instr::istate::ClearAlarmRequest req;
    req.set_alarm_id(static_cast<uint32_t>(id));
    req.set_owner(kOwner); // must match the raise
    grpc::ClientContext ctx;
    instr::istate::AlarmAck ack;
    const grpc::Status s = st->ClearAlarm(&ctx, req, &ack);
    if (!s.ok())
        std::cerr << "[hal] ClearAlarm failed: " << s.error_message() << "\n";
}

#else // !ARCHES_WITH_INSTRUMENTATION

// Built without the instrumentation package: a supported configuration, not a
// degraded one. HAL is linked by 36 standalone test binaries that must build
// with nothing extra installed.
void raise(AlarmId, const std::string&) {}
void clear(AlarmId) {}

#endif

} // namespace arches::hal::instrumentation
