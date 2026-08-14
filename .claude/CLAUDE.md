# Arches-HAL

C++17 hardware abstraction and validation layer for the Jetson. Owns the canonical proto/gRPC
contract the rest of the fleet links against.

Remote: `git@github.com:soccentric-arches/Arches-HAL.git`
Target: NVIDIA Jetson (Tegra, L4T/JetPack) + STM32 real-time co-processor.

Two things at once: a hardware test-suite framework (one binary per module, uniform CLI), and the
canonical proto/gRPC contract source for the fleet.

## How you work here

* **Commands**: `.claude/commands/` — build, test, cross-test, proto-contract, validate.
* **Skills** (`.pi/skills/`) — on-demand capability packages. Use `/skill:name` or let the agent
  load them automatically when a task matches.
  * `coding-standards` — clang-format, cppcheck, MISRA C++ compliance
  * `proto-contracts` — protobuf/gRPC contract maintenance
  * `remote-test` — deploying and running tests on real Jetson hardware
  * `add-hal-module` — adding or restructuring a hardware module
  * `native-test` — running CTest in the local build tree
  * `validate` — running the validation gate
  * `hal-module-manager` — full module lifecycle (add, remove, restructure)
  * `project-specs` — detailed project layout and architecture reference

* **Prompt templates** (`.pi/prompts/`) — slash commands for common tasks:
  * `/build` — configure and build HAL (native)
  * `/test` — run the native CTest suite
  * `/cross-test` — build and test for ARM64 Jetson target via Docker/QEMU
  * `/proto-contract` — validate protobuf/gRPC contracts
  * `/validate` — run the validation gate (quick | full)

* **Write**: a new module is a `TestBase` subclass plus `runTestApp<T>()` wiring, matching the existing 35. That is what keeps every module binary's CLI identical.
* **Test**: CTest via `enable_testing()`. Build is `-Wall -Wextra -Wpedantic -Werror` — warnings are failures.

## How you validate

* Gate: `./.claude/validate.sh quick` (seconds) or `full` (adds build + tests). Run it before you claim done.
* Exit codes: `0` pass, `1`-`76` fail, `77` a check never ran. **77 is not a pass.**
* The gate and everything under `.claude/lib`, `hooks`, `commands`, `settings.json` is human-owned. Run it, never edit it.

## Layout

* `src/<module>_lib/` — 35 built module libraries. Each pairs with `tests/<module>/` and `protos/modules/<id>.proto`.
* `src/lib_base.h/.cpp` — shared `TestBase` / `TestOutput` / `hal::Status` / `hal::Value` framework. Every module subclasses `TestBase`.
* `src/test_runner.h` — `runTestApp<T>()`, the template giving every module binary the same CLI.
* `src/platform_arches.h` — Jetson platform constants: SoC family, L4T/JetPack OS, CPU core range 8-12, 95C CPU/GPU thermal max, GPU render node, devfreq and thermal-zone paths, and the STM32 UART/SPI framing constants (SOF `0xA5`, GPIO opcodes, ping, heartbeat, 115200 baud, 1MHz SPI, 100ms timeout).
* `src/stm32_transport.h/.cpp` + `src/mcu_interface_lib/` — the STM32 co-processor link (`Stm32Transport`). Covers GPIO/SPI/UART/DMS pass-through, secondary-MCU redundancy, failover, sync.
* `protos/hal_service.proto` — stable gRPC contract. Module config/result are `google.protobuf.Any`, so adding a module never changes an RPC signature.
* `protos/client_service.proto` — bidi streaming contract, package `arches.stream`.
* `protos/common.proto`, `protos/modules/*.proto` (49), `protos/registry/modules.yaml` — the machine-readable module manifest.

## The 35 modules

audio_interfaces, camera_interfaces, camera_module3, clock_and_timing, communication_interface,
cpu, debug_and_development_interfaces, deep_learning_accelerator, display_interfaces,
environmental, ethernet, ethernet_switch, external_connector, functional_safety, gnss, gpio, gpu,
imu, mcu_interface, pcie, poe_hat, power_management, reliability, security_features, sense_hat,
serializer_deserializer, software_interface, storage, system_integration, system_memory,
thermal_management, usb, video_encode_decode, vision_accelerator, watchdog_and_reset.

## Do

* Treat this repo as the **only** source of generated proto/gRPC symbols. Client and Server link it; nobody regenerates.
* Keep `protos/registry/modules.yaml` in sync with `src/<module>_lib/` and `protos/modules/*.proto` when adding a module — every consumer reads it.
* Give new modules the `TestBase` + `runTestApp<T>()` shape so the one-shot/monitoring/long-term/monitor-test/event CLI stays uniform.
* Write: a new module is a `TestBase` subclass plus `runTestApp<T>()` wiring, matching the existing 35. That is what keeps every module binary's CLI identical.
* Test: CTest via `enable_testing()`. Build is `-Wall -Wextra -Wpedantic -Werror` — warnings are failures.

## Don't

* **Don't delete the 14 protos with no `src/` implementation.** Reserved future hardware coverage, not dead code. Proto count 49 > module count 35 on purpose.
* **Don't rename the `arches.stream` package** in `protos/client_service.proto`. Renaming it once already required Client and Server to change in lockstep; any further rename needs the same cross-repo approval.
* **Don't delete `src/platform_rpi5.h`.** Compatibility alias shim for modules not yet ported off the legacy platform header.
* **Don't leave fork-lineage naming.** The tree should read as natively Arches. Leftovers are bugs, not intentional debt.
* **Don't edit `.claude/validate.sh`, `.claude/lib/`, `.claude/hooks/`, `.claude/commands/`, `.claude/settings.json`, or `tools/validate/`** — those are human-owned. Run them, never edit them.

<!-- BEGIN loop-engine -->
## Do's and Don'ts (maintained by the Loop Engine)

Appended by the Loop Engine on loop close; curated by a human when approving a loop out of in-review/. Nothing outside this fence is engine-writable.

<!-- END loop-engine -->
