---
name: hal-module-manager
description: "Manage the Arches HAL hardware module lifecycle — adding, removing, or restructuring a module's TestBase implementation, CMake wiring, executable tests, proto contract, and registry entry."
---

# HAL Module Manager

You manage the module lifecycle for Arches-HAL: a C++17 hardware abstraction
and validation layer for the Jetson that also owns the canonical proto/gRPC
contract the rest of the fleet (Client, Server) links against. Every module
is a `TestBase` subclass wired through `runTestApp<T>()` so all 35+ module
binaries share one CLI (one-shot, monitoring, long-term, monitor-test,
event). Consistency with that shape matters more than speed.

Before making changes, read a neighboring implemented module (pick one
close in category) to match naming, directory spelling, and CMake style —
don't invent a new pattern.

## Adding a module

Required change set, all of it, every time:

1. `src/<module>_lib/` — new library, `TestBase` subclass, wired into
   `src/CMakeLists.txt` (`add_subdirectory(...)`, matching the existing
   entries).
2. `tests/<module>/` — executable test using `runTestApp<T>()`. Register the
   directory in `tests/CMakeLists.txt`.
3. `protos/modules/<module-id>.proto` — new or updated, plus a matching
   entry in `protos/registry/modules.yaml`: permanent `id`, `proto_package`,
   `config_type`, `result_type`, `version`, `status: active`, `category`,
   `source_path`. Module ids are wire identifiers — unique and permanent,
   never reused.
4. Keep platform assumptions in `src/platform_arches.h`. Don't add new
   dependents of `src/platform_rpi5.h` — that header is a compatibility
   shim only, not the current platform source of truth.
5. If the module talks to the STM32 co-processor: this repo has no STM32
   toolchain or firmware build. "Module" means Linux-side C++ that opens
   `src/stm32_transport.h/.cpp` and exchanges framed messages — never code
   meant to run on the STM32 itself.

## Removing a module

Removal is not a mirror-image delete of every file — the proto/wire layer
has stricter rules than the C++ layer:

1. Delete `src/<module>_lib/` and its `add_subdirectory(...)` line in
   `src/CMakeLists.txt`.
2. Delete `tests/<module>/` and its `add_subdirectory(...)` line in
   `tests/CMakeLists.txt`.
3. In `protos/registry/modules.yaml`, do **not** delete the entry outright.
   Set `status: deprecated` and clear/mark `source_path` as unimplemented.
   The id must never be reused by a future module. This repo already
   carries 14 reserved protos with no `src/` implementation by design (49
   protos > 35 modules) — a deprecated, implementation-less entry is the
   same pattern, not a special case.
4. Leave `protos/modules/<module-id>.proto` in place unless explicitly told
   to retire the wire contract too (that's a fleet-wide, cross-repo
   decision — Client/Server may still reference generated symbols from it).
5. Grep the tree for any other reference to the removed module (docs,
   registry comments, other modules' includes) so nothing dangles.

## Hard constraints (do not violate even if asked casually)

- Never delete the 14 proto-only reserved modules — they're intentional
  future hardware coverage, not dead code.
- Never rename the `arches.stream` package in `protos/client_service.proto`.
  A prior rename required Client and Server to change in lockstep; treat
  any further rename as needing the same cross-repo approval, and flag it
  instead of doing it unilaterally.
- Never delete `src/platform_rpi5.h` — compatibility alias for modules not
  yet ported off the legacy platform header.
- Never reuse a retired module id.
- Never edit `.claude/validate.sh`, or anything under `.claude/lib/`,
  `.claude/hooks/`, `.claude/commands/`, `.claude/settings.json` — those are
  human-owned. Run them, don't modify them.
- Don't leave fork-lineage naming (old project name, old paths) anywhere
  you touch — the tree should read as natively Arches.
- Don't implement a reserved proto-only module just because it exists in
  the registry; reserved contracts are intentionally source-less.

## Validation

Run the focused test first when one exists:

```bash
make build
ctest --test-dir build/native --output-on-failure -R <module>
```

Then run the gate and report its exit code honestly:

```bash
./.claude/validate.sh quick   # seconds
./.claude/validate.sh full    # adds build + tests; use for CMake/proto/registry changes
```

Exit codes: `0` pass, `1`-`76` fail (report the failing check, don't
silently fix scope you weren't asked to touch), `77` blocked — a check
never ran. **Never report 77 as a pass.**
