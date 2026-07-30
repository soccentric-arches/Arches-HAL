# Arches-HAL

C++17 hardware abstraction and validation layer for the Jetson. Owns the canonical proto/gRPC
contract the rest of the fleet links against.

## How you work here

* Commands: `.claude/commands/` — build, test, cross-test, proto-contract, validate.
* Write: a new module is a `TestBase` subclass plus `runTestApp<T>()` wiring, matching the existing 35. That is what keeps every module binary's CLI identical.
* Test: CTest via `enable_testing()`. Build is `-Wall -Wextra -Wpedantic -Werror` — warnings are failures.

## How you validate

* Gate: `./.claude/validate.sh quick` (seconds) or `full` (adds build + tests). Run it before you claim done.
* Exit codes: `0` pass, `1`-`76` fail, `77` a check never ran. **77 is not a pass.**
* The gate and everything under `.claude/lib`, `hooks`, `commands`, `settings.json` is human-owned. Run it, never edit it.

## Do

* Treat this repo as the **only** source of generated proto/gRPC symbols. Client and Server link it; nobody regenerates.
* Keep `protos/registry/modules.yaml` in sync with `src/<module>_lib/` and `protos/modules/*.proto` when adding a module — every consumer reads it.
* Give new modules the `TestBase` + `runTestApp<T>()` shape so the one-shot/monitoring/long-term/monitor-test/event CLI stays uniform.

## Don't

* **Don't delete the 14 protos with no `src/` implementation.** Reserved future hardware coverage, not dead code. Proto count 49 > module count 35 on purpose.
* **Don't rename the `arches.stream` package** in `protos/client_service.proto`. Renaming it once already required Client and Server to change in lockstep; any further rename needs the same cross-repo approval.
* **Don't delete `src/platform_rpi5.h`.** Compatibility alias shim for modules not yet ported off the legacy platform header.
* **Don't leave fork-lineage naming.** The tree should read as natively Arches. Leftovers are bugs, not intentional debt.

<!-- BEGIN loop-engine -->
## Do's and Don'ts (maintained by the Loop Engine)

Appended by the Loop Engine on loop close; curated by a human when approving a loop out of in-review/. Nothing outside this fence is engine-writable.

<!-- END loop-engine -->
