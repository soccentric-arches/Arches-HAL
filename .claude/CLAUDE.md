# CLAUDE.md - Arches-HAL

Master SoCcentric CLAUDE.md applies. This file adds local knowledge only. See [project_specs.md](project_specs.md) for what this repo is and its architecture.

## Commands

* Build: `make build` (CMake 3.10+, Ninja, `-Wall -Wextra -Wpedantic -Werror`)
* Test: `make test` (CTest via `enable_testing()`)
* Cross-build: Docker/QEMU ARM64 (`docker/`)
* Deploy: `make deploy-a` / `deploy-b` (SSH, default user `nvidia`)
* Package: CPack → DEB/RPM/IPK (`arches-hal`), installs headers to `arches_hal/`
* Lint/docs: optional clang-format, cppcheck, Doxygen targets

## Do

* Treat this repo as the single source of generated proto/gRPC symbols. Arches-Client and Arches-Server both consume it via `add_subdirectory(../Arches-HAL ...)` and link `hal_protos_shared`/`acu_full_test` — never regenerate protos in a sibling repo (ODR/duplicate-symbol risk).
* Keep `protos/registry/modules.yaml` in sync with `src/<module>_lib/` and `protos/modules/*.proto` when adding a module — it's the machine-readable map every consumer (Client's `ModuleRegistry`) reads.
* New modules get a `TestBase` subclass + `runTestApp<T>()` wiring, matching the existing 35 — this keeps the CLI (one-shot/monitoring/long-term/monitor-test/event) uniform across all module binaries.

## Don't

* Don't be surprised the proto count (49) exceeds the built module count (35). 14 protos (accelerometer, battery, bluetooth, can_bus, emmc, i2c, mipi_csi/dsi, nvme, pwm_output, rs485, rtc, sd_card, spi, analog_input, uart) have no corresponding `src/` implementation yet — they're reserved/future hardware coverage, not dead code to delete.
* Don't rename the `arches.stream` proto package in `protos/client_service.proto` again without cross-repo coordination — it was renamed from the legacy fork-lineage package name as an approved, coordinated breaking change (Arches-Client and Arches-Server updated to match in the same pass); any further rename needs the same cross-repo approval.
* Don't assume `src/platform_rpi5.h` is dead history to clean up casually — it's an explicit compatibility-alias shim for modules not yet ported off the original legacy Raspberry Pi 5 platform header. Check it before deleting.
* This repo was forked from a shared Raspberry Pi 5 / BCM2712 reference template. The fork-lineage naming debt has been fully rebranded to Arches fleet-wide — the tree should read as natively Arches with no trace of the fork history. If you spot leftover fork-lineage naming in the future, treat it as a bug to fix, not intentional debt.

## Validation gate - .claude/validate.sh

```
./.claude/validate.sh quick   # seconds: `make cppcheck` (this repo has no `make lint` target), clang-format, and the proto-contract checks that keep Client and Server linkable
./.claude/validate.sh full    # + configure, build, full CTest set
```

Exit codes follow the fleet contract: 0 pass, 1-76 fail, 77 means a check never ran (missing toolchain) - which is not a pass. Runs automatically via the Stop hook in .claude/settings.json, on demand via /validate, and as part of any loop the engine executes here.

Human-owned: the Loop Engine runs this script and never edits it. If a check is wrong, fix it yourself or raise it in the loop's Lessons Learned.

<!-- BEGIN loop-engine -->
## Do's and Don'ts (maintained by the Loop Engine)

Appended by the Loop Engine on loop close; curated by a human when approving a loop out of in-review/. Nothing outside this fence is engine-writable.

<!-- END loop-engine -->
