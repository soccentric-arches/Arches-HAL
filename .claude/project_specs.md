# Arches-HAL — specs

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
