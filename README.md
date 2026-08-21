# Arches-HAL

## Project Summary

Arches-HAL is the C++17 hardware abstraction and validation layer shared by the Arches client and target platforms. Protocol definitions in `protos/` describe common messages, client and HAL services, and module contracts ranging from accelerometers and cameras to storage, networking, power, safety, and watchdog functions. The module registry manifest provides the interface inventory used by generated and hand-written code. CMake builds protocol artifacts and the library implementation under `src/`.

The source tree includes common provider and module helpers, Arches and Raspberry Pi 5 platform bindings, STM32 transport support, and module libraries for the declared interfaces. These implementations bridge Linux facilities and MCU or peripheral transports into a consistent test contract. Doxygen inputs and Docker/toolchain files support documentation and embedded builds.

`tests/` contains per-module executable test programs and CMake definitions for many interface families, plus focused GPU, pinmux, and STM32 transport cases. Many tests interact with real hardware or platform devices; test-binary presence is not equivalent to successful target execution. The Makefile and `script/run-all-targets.sh` expose build and execution workflows.

The HAL is strongly platform-dependent: compiler flags, device nodes, peripheral availability, and transport wiring affect runtime results. Review generated protocol freshness, target permissions, module registration, and test fixture assumptions before release. Also compare every supported board’s expected buses, permissions, timing, and failure reporting with its fixture configuration. No manual verification is implied by a compiled test binary.

## Features

| # | Name / Description | Implemented? | Test Coverage? | Manual Test |
|---:|---|---|---|---|
| 1 | Common, client, and HAL protobuf services | Implemented | Protocol CMake targets | [ ] Manual verification required |
| 2 | Module registry and generated contracts | Implemented | Partial: module targets | [ ] Manual verification required |
| 3 | Provider and module helper infrastructure | Implemented | Partial | [ ] Manual verification required |
| 4 | Arches and Raspberry Pi 5 bindings | Implemented | Partial: platform-dependent | [ ] Manual verification required |
| 5 | STM32 transport | Implemented | `tests/stm32_transport` | [ ] Manual verification required |
| 6 | Camera, display, GPU, and vision interfaces | Implemented | Per-module tests | [ ] Manual verification required |
| 7 | CPU, memory, storage, PCIe, USB, networking | Implemented | Per-module tests | [ ] Manual verification required |
| 8 | GPIO, IMU, environmental, power, safety | Implemented | Per-module tests | [ ] Manual verification required |
| 9 | Doxygen API documentation inputs | Implemented | Unknown | [ ] Manual verification required |
| 10 | Full validation on every supported target | Partial | Not established from source alone | [ ] Manual verification required |

## Validation

`make build`; `make test`; `./script/run-all-targets.sh`. Target prerequisites and hardware access are required.
