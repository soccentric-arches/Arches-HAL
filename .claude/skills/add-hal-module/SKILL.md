---
name: add-hal-module
description: "Use when: adding or substantially restructuring an Arches HAL hardware module, its TestBase implementation, CMake targets, executable tests, or registry entry."
---

# Add An HAL Module

Arches HAL modules are C++17 hardware validation libraries with a uniform test
application CLI. Follow a neighboring implemented module before choosing target
names or directory spelling.

## Required Change Set

1. Add the module library under src/ as a TestBase subclass and wire it into
   src/CMakeLists.txt.
2. Add an executable test under tests/, using runTestApp<T>() so it supports
   one-shot, monitoring, long-term, monitor-test, and event.
3. Register the test directory in tests/CMakeLists.txt.
4. Add or update protos/modules/<module-id>.proto and its matching entry in
   protos/registry/modules.yaml, including the permanent id, package, config
   type, result type, version, category, and source path.
5. Keep platform assumptions in src/platform_arches.h. Retain
   src/platform_rpi5.h only as the compatibility alias while dependent modules
   still include it.

## STM32 Scope (read this before touching mcu_interface_lib or stm32_transport)

This repo builds and runs only for the Linux side (Jetson/Tegra, L4T/JetPack).
There is no STM32 firmware, no STM32 toolchain, and no STM32-target build here.
The STM32 is a real-time co-processor reached exclusively as a peer device over
UART/SPI framing (src/stm32_transport.h/.cpp, SOF byte, GPIO opcodes, ping,
heartbeat — see src/platform_arches.h). A "module" that talks to the STM32
means: Linux-side C++ code that opens the transport, sends/receives framed
messages, and validates the response — never code that is compiled for or
flashed onto the STM32 itself.

## Validation

Start with the focused test when one exists:

```bash
make build
ctest --test-dir build/native --output-on-failure -R <module>
```

Then run ./.claude/validate.sh quick. Use the full tier for changes that
touch shared framework code, CMake registration, or proto contracts.

Do not implement a reserved proto-only module merely because it appears in the
registry. Reserved contracts are intentionally allowed to have no src/
implementation.