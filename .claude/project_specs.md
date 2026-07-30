# Arches-HAL — specs

Remote: `git@github.com:SiliconCentric/Arches-HAL.git`
Parent project: Arches (target hardware: NVIDIA Jetson, Tegra family, L4T/JetPack OS, with an STM32 real-time co-processor)
Leaf type: `<Project>-HAL`
Status: REAL — actively built out

## What this is

No prose README, but a fully real C++17 hardware abstraction/validation layer: `CMakeLists.txt`, `Doxyfile.in`, `Makefile`, `cmake/`, `docker/`, `protos/`, `registry/`, `src/`, `tests/`. It is both a hardware test-suite framework (one binary per module, uniform CLI) and the canonical proto/gRPC contract source for the whole Arches fleet — Arches-Client and Arches-Server both link it as a sibling subproject and never regenerate protos themselves.

## Architecture

* `src/<module>_lib/` — 35 built module libraries, each pairing a `src/<module>_lib/`, `tests/<module>/`, and `protos/modules/<id>.proto`: audio_interfaces, camera_interfaces, camera_module3, clock_and_timing, communication_interface, cpu, debug_and_development_interfaces, deep_learning_accelerator, display_interfaces, environmental, ethernet, ethernet_switch, external_connector, functional_safety, gnss, gpio, gpu, imu, mcu_interface, pcie, poe_hat, power_management, reliability, security_features, sense_hat, serializer_deserializer, software_interface, storage, system_integration, system_memory, thermal_management, usb, video_encode_decode, vision_accelerator, watchdog_and_reset.
* `src/lib_base.h/.cpp` — shared `TestBase`/`TestOutput`/`hal::Status`/`hal::Value` (variant) framework. Every module subclasses `TestBase`.
* `src/test_runner.h` — generic `runTestApp<T>()` template giving every module binary the same CLI: one-shot, monitoring, long-term, monitor-test, event.
* `src/platform_arches.h` — canonical Jetson/Tegra platform constants: `PLATFORM_NAME = "Arches"`, `PLATFORM_SOC_FAMILY = "NVIDIA Tegra"`, `PLATFORM_OS = "L4T / JetPack"`, CPU core range 8-12, CPU/GPU thermal max 95C, GPU render node `/dev/dri/renderD128`, Jetson-specific devfreq nodes (`17000000.ga10b`, `gpu.0`), three thermal zone paths, and the STM32 UART/SPI framing constants (SOF byte 0xA5, GPIO direction/write/read/pull opcodes, ping, heartbeat; default baud 115200 / SPI 1MHz / timeout 100ms).
* `src/stm32_transport.h/.cpp`, `src/mcu_interface_lib/` — the STM32 real-time co-processor link (`Stm32Transport` class). Tests cover GPIO/SPI/UART/DMS pass-through, secondary-MCU redundancy, failover, and synchronization (`tests/mcu_interface/`, `tests/stm32_transport/`).
* `protos/hal_service.proto` — stable gRPC contract; module config/result are `google.protobuf.Any`-typed so the RPC signature never has to change when modules are added.
* `protos/client_service.proto` — bidi streaming contract, package `arches.stream` (renamed from the legacy fork-lineage package name as an approved breaking change; Arches-Client and Arches-Server updated in lockstep).
* `protos/common.proto`, `protos/modules/*.proto` (49 files), `protos/registry/modules.yaml` — machine-readable manifest mapping module id to proto package/types/version/source path.

Part of the Arches project in the project-managers fleet. See `docs/project-managers.md` for the full 9-project / 8-leaf hierarchy and shared conventions.
