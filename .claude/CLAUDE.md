# CLAUDE.md - Arches-HAL

Remote: `git@github.com:SiliconCentric/Arches-HAL.git`
Parent project: Arches (target hardware: NVIDIA Jetson (Tegra family, L4T/JetPack OS) application processor with an STM32 real-time co-processor.)
Leaf type: `<Project>-HAL`
Status: REAL - actively built out

Master SoCcentric CLAUDE.md applies. This file adds local knowledge only.

No prose README, but a fully real C++17 hardware abstraction/validation layer: `CMakeLists.txt`, `Doxyfile.in`, `Makefile`, `cmake/`, `docker/`, `protos/`, `registry/`, `src/`, `tests/`. It is both a hardware test-suite framework (one binary per module, uniform CLI) and the canonical proto/gRPC contract source for the whole Arches fleet - `Arches-Client` and `Arches-Server` both link it as a sibling subproject and never regenerate protos themselves.

# Commands

* Build: `make build` (CMake 3.10+, Ninja, `-Wall -Wextra -Wpedantic -Werror`)
* Test: `make test` (CTest via `enable_testing()`)
* Cross-build: Docker/QEMU ARM64 (`docker/`)
* Deploy: `make deploy-a` / `deploy-b` (SSH, default user `nvidia`)
* Package: CPack -> DEB/RPM/IPK (`arches-hal`), installs headers to `arches_hal/`
* Lint/docs: optional `clang-format`, `cppcheck`, Doxygen targets

# Architecture

* `src/<module>_lib/` - 35 built module libraries, each pairing a `src/<module>_lib/`, `tests/<module>/`, and `protos/modules/<id>.proto`: audio_interfaces, camera_interfaces, camera_module3, clock_and_timing, communication_interface, cpu, debug_and_development_interfaces, deep_learning_accelerator, display_interfaces, environmental, ethernet, ethernet_switch, external_connector, functional_safety, gnss, gpio, gpu, imu, mcu_interface, pcie, poe_hat, power_management, reliability, security_features, sense_hat, serializer_deserializer, software_interface, storage, system_integration, system_memory, thermal_management, usb, video_encode_decode, vision_accelerator, watchdog_and_reset.
* `src/lib_base.h`/`.cpp` - shared `TestBase`/`TestOutput`/`hal::Status`/`hal::Value` (variant) framework. Every module subclasses `TestBase`.
* `src/test_runner.h` - generic `runTestApp<T>()` template that gives every module binary the same CLI: `one-shot`, `monitoring`, `long-term`, `monitor-test`, `event`.
* `src/platform_arches.h` - canonical Jetson/Tegra platform constants: `PLATFORM_NAME = "Arches"`, `PLATFORM_SOC_FAMILY = "NVIDIA Tegra"`, `PLATFORM_OS = "L4T / JetPack"`, CPU core range 8-12, CPU/GPU thermal max 95C, GPU render node `/dev/dri/renderD128`, Jetson-specific devfreq nodes (`17000000.ga10b`, `gpu.0`), three thermal zone paths, and the STM32 UART/SPI framing constants (SOF byte `0xA5`, GPIO direction/write/read/pull opcodes, ping, heartbeat; default baud 115200 / SPI 1MHz / timeout 100ms).
* `src/stm32_transport.h`/`.cpp`, `src/mcu_interface_lib/` - the STM32 real-time co-processor link (`Stm32Transport` class). Tests cover GPIO/SPI/UART/DMS pass-through, secondary-MCU redundancy, failover, and synchronization (`tests/mcu_interface/`, `tests/stm32_transport/`).
* `protos/hal_service.proto` - stable gRPC contract; module config/result are `google.protobuf.Any`-typed so the RPC signature never has to change when modules are added.
* `protos/client_service.proto` - bidi streaming contract, `package arches.stream` (renamed from the legacy fork-lineage package name as an approved breaking change; Arches-Client and Arches-Server updated in lockstep).
* `protos/common.proto`, `protos/modules/*.proto` (49 files), `protos/registry/modules.yaml` - machine-readable manifest mapping module id to proto package/types/version/source path.

# Do

* Treat this repo as the single source of generated proto/gRPC symbols. `Arches-Client` and `Arches-Server` both consume it via `add_subdirectory(../Arches-HAL ...)` and link `hal_protos_shared`/`acu_full_test` - never regenerate protos in a sibling repo (ODR/duplicate-symbol risk).
* Keep `protos/registry/modules.yaml` in sync with `src/<module>_lib/` and `protos/modules/*.proto` when adding a module - it's the machine-readable map every consumer (Client's `ModuleRegistry`) reads.
* New modules get a `TestBase` subclass + `runTestApp<T>()` wiring, matching the existing 35 - this keeps the CLI (`one-shot`/`monitoring`/`long-term`/`monitor-test`/`event`) uniform across all module binaries.

# Don't

* Don't be surprised the proto count (49) exceeds the built module count (35). 14 protos (accelerometer, battery, bluetooth, can_bus, emmc, i2c, mipi_csi/dsi, nvme, pwm_output, rs485, rtc, sd_card, spi, analog_input, uart) have no corresponding `src/` implementation yet - they're reserved/future hardware coverage, not dead code to delete.
* Don't rename the `arches.stream` proto package in `protos/client_service.proto` again without cross-repo coordination - it was renamed from the legacy fork-lineage package name as an approved, coordinated breaking change (Arches-Client and Arches-Server updated to match in the same pass); any further rename needs the same cross-repo approval.
* Don't assume `src/platform_rpi5.h` is dead history to clean up casually - it's an explicit compatibility-alias shim (comment: "modules being migrated to shared Arches platform header") for modules not yet ported off the original legacy Raspberry Pi 5 platform header. Check it before deleting.
* This repo was forked from a shared Raspberry Pi 5 / BCM2712 reference template. The fork-lineage naming debt (old package/image/volume names, RPi-flavored comments) has been fully rebranded to Arches fleet-wide - the tree should read as natively Arches with no trace of the fork history. If you spot leftover fork-lineage naming in the future, treat it as a bug to fix, not intentional debt.

---
Part of the `Arches` project in the `project-managers` fleet. See `docs/project-managers.md` for the full 9-project / 8-leaf hierarchy and shared conventions.

# Validation gate - .claude/validate.sh

```
./.claude/validate.sh quick   # seconds: `make cppcheck` (this repo has no `make lint` target), clang-format, and the proto-contract checks that keep Client and Server linkable
./.claude/validate.sh full    # + configure, build, full CTest set
```

Exit codes follow the fleet contract: `0` pass, `1`-`76` fail, `77` means a check never ran (missing toolchain) - which is not a pass. Runs automatically via the `Stop` hook in `.claude/settings.json`, on demand via `/validate`, and as part of any loop the engine executes here.

Human-owned: the Loop Engine runs this script and never edits it. If a check is wrong, fix it yourself or raise it in the loop's Lessons Learned.

<!-- BEGIN loop-engine -->
## Do's and Don'ts (maintained by the Loop Engine)

*Appended by the Loop Engine on loop close; curated by a human when approving a loop out of `in-review/`. Nothing outside this fence is engine-writable.*

<!-- END loop-engine -->
