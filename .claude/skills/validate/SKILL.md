---
name: validate
description: "Use when: validating or verifying Arches HAL changes before considering a task done - running `.claude/validate.sh`, interpreting its checks and exit codes, or deciding between the quick and full tiers."
---

# Validate & Verify Arches HAL Changes

`.claude/validate.sh` is the repo's validation gate (see `.claude/lib/validate-lib.sh`
for the shared `assert`/`check`/`report`/`tier_or_die` helpers). It is
human-owned: fix a check or raise it in the loop's Lessons Learned, never edit
around it. It also runs automatically via the `Stop` hook and via `/validate`.

```bash
./.claude/validate.sh quick   # seconds - repo-shape + static checks, safe as an edit-time gate
./.claude/validate.sh full    # minutes - + cmake configure, native build, full CTest set
```

## Exit code contract

| Code | Meaning |
|---|---|
| `0` | All checks passed |
| `1`-`76` | At least one check failed |
| `77` | A check never ran (missing toolchain) — treat as **blocked**, not a pass |

## What `quick` checks

1. **Proto contracts present** — `protos/hal_service.proto`,
   `protos/client_service.proto`, `protos/common.proto` all exist.
2. **No stray generated protos** — no `*.pb.cc` under `src/` newer than
   `protos/hal_service.proto`. HAL commits generated code intentionally
   because Client and Server link its symbols; a stray regeneration here is
   how ODR/duplicate-symbol link errors start downstream.
3. **Every top-level `.proto` named in `protos/CMakeLists.txt`** — top-level
   protos (`COMMON_PROTO`, `SERVICE_PROTO`, `CLIENT_SERVICE_PROTO`) are named
   individually; only `protos/modules/*.proto` is globbed. A new top-level
   `.proto` is silently dropped from the build unless added there.
3b. **`modules/*.proto` still glob-collected** — guards that the glob pattern
   in `protos/CMakeLists.txt` wasn't accidentally narrowed or removed.
4. **CMake configure parses** — `cmake -GNinja -S . -B build/native` succeeds
   (requires `cmake`, `ninja`).
5. **cppcheck static analysis** — `make cppcheck` (requires `cppcheck`, `cmake`,
   `ninja`). HAL has no `make lint` target, unlike Client/Server/Configuration/
   HQ/OS — cppcheck is the nearest equivalent here, and the check requires the
   tool to actually be present rather than silently passing when it's absent.
   See the [coding-standards](../coding-standards/SKILL.md) skill for the
   cppcheck/clang-format setup in depth, and for MISRA compliance guidance.
6. **clang-format clean** — `--dry-run --Werror` over `src/` and `protos/`
   (`.cc`/`.h`/`.cpp`), requires `clang-format`.

## What `full` adds

7. **Builds clean** — `make build` (native tree).
8. **Full CTest suite** — `make test`, the complete ~36-executable set in
   `build/native`. Unlike Client/Server (which filter by label because they
   link HAL's unbuilt `EXCLUDE_FROM_ALL` targets), HAL's own `make test` runs
   everything directly — that's correct here, not a gap.

Note: `full` only exercises the **native** tree. Cross/ARM64 correctness is a
separate, complementary step — see the [build](../build/SKILL.md) skill's
`make docker-build` / `make docker-test` for target-accurate verification
(e.g. STM32 transport, platform constants, or anything ABI-sensitive).

## Recommended workflow

- Every change: `./.claude/validate.sh quick`.
- Anything touching shared framework code (`src/lib_base.*`, `src/test_runner.h`),
  CMake registration, or proto/gRPC contracts: `./.claude/validate.sh full`.
- Before declaring cross-platform-sensitive work done: also run
  `make docker-build && make docker-test` (see [build](../build/SKILL.md)).
- Treat exit `77` as "fix your toolchain and re-run", never as green.
