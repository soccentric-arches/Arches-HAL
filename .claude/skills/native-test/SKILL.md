---
name: native-test
description: "Use when: running, filtering, or debugging CTest in the local/native Arches HAL build tree (`build/native`) - `make test`, targeted `-R`/`-E` selection, listing tests, or invoking a single test binary directly."
---

# Run Native (Local) Tests

Native tests run against build/native, the host-architecture CMake/Ninja
tree (as opposed to build/cross, the ARM64 tree — see the
[build](../build/SKILL.md) skill).

## Standard run

```bash
make build   # configure (if needed) + ninja
make test    # cd build/native && ctest --output-on-failure -j$(JOBS)
```

## Targeted / filtered runs

Test names follow test_<module> (e.g. test_gpio, test_mcu_interface),
defined per-module in tests/<module>/CMakeLists.txt via add_test(NAME ...
COMMAND ...). Use ctest directly for finer control than make test gives:

```bash
ctest --test-dir build/native --output-on-failure -R <module>     # run tests matching regex
ctest --test-dir build/native --output-on-failure -E <module>     # exclude tests matching regex
ctest --test-dir build/native -N                                   # list tests without running
ctest --test-dir build/native --output-on-failure -j$(nproc)       # explicit parallelism
```

Each test has a TIMEOUT property set in its tests/<module>/CMakeLists.txt
(commonly 300s — check the specific module if a long-running test needs more).

## Running a binary directly (bypass CTest)

Useful when iterating on a single module or need to pass CLI args/modes that
CTest's default invocation doesn't:

```bash
LD_LIBRARY_PATH=build/native/lib build/native/bin/test_<module> [mode] [args...]
# or, via the Makefile helper (defaults to running every binary if no name given):
make run                # run all built test apps locally
make run test_<module>  # run one, with optional trailing args
```

Every test binary shares the same CLI surface via runTestApp<T>()
(src/test_runner.h): one-shot, monitoring, long-term, monitor-test,
event.

## Relationship to validation

./.claude/validate.sh full (see the [validate](../validate/SKILL.md) skill)
runs make build + make test as part of its full tier — the native ctest
run is the authoritative pass/fail signal there. Prefer the direct ctest -R
<module> form over the whole validate script when only iterating on one
module's test during development.
