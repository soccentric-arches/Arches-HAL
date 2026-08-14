---
name: project-specs
description: "Reference for Arches-HAL project layout, architecture, module list, and platform constants. Use when you need the full picture of how the repo is structured."
---

# Arches-HAL — Project Specs

Remote: `git@github.com:soccentric-arches/Arches-HAL.git`
Target: NVIDIA Jetson (Tegra, L4T/JetPack) + STM32 real-time co-processor.

Two things at once: a hardware test-suite framework (one binary per module, uniform CLI), and the
canonical proto/gRPC contract source for the fleet.

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

## Fleet context

Arches-HAL is one of 9 leaf repos in the Arches project. The fleet map:

| Leaf | Runs on | Role |
|---|---|---|
| Arches-HAL | Jetson | Hardware abstraction, canonical proto/gRPC contract, STM32 link |
| Arches-Client | Jetson | IV&V client; drives HAL and streams results |
| Arches-Server | x86 host | gRPC peer; run control and SQLite result ingestion |
| Arches-PySDK | Any | Python SDK for HAL's service contract |
| Arches-Configuration | Jetson | On-device configuration UI and `meta-webconfig` layer |
| Arches-OS | Jetson | Yocto/kas image, board support, and runtime middleware |
| Arches-Dashboard | Any | Standalone Next.js dashboard |
| Arches-HQ | Cloud | Fleet-management API, database, UI, pairing, and remote operations |
| Arches-Hardware | - | Read-only, human-owned schematics and BOM; no build or gate |

## Dependency rules

* HAL owns the C++ proto/gRPC contract. Client and Server consume HAL as sibling CMake subprojects and must not regenerate its protos; doing so creates duplicate-symbol/ODR failures.
* A HAL proto change requires compatible Client and Server changes. Proto or package renames are breaking changes and require explicit approval.
* PySDK intentionally regenerates its own stubs with `make gen`; `make gen-check` detects stale output. Any `hal_service.proto` change affects it.
* Client and Server are a matched pair using `client_service.proto` in package `arches.stream`.
* Configuration is not proto-coupled. It manages system configuration through systemd, zbus, RAUC, and journald and supplies OS's `meta-webconfig` layer.
* OS provides the runtime substrate: gRPC, Protobuf, libgpiod, and image integration. This coupling is runtime, not a direct source dependency.
* HQ has no shared wire contract with Client, Server, or PySDK. Its integration is Configuration's outbound device-pairing agent.
* Hardware is never a change or verify scope in a loop.
