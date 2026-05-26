# Arches-HAL – Hardware Abstraction Library for NVIDIA Tegra + STM32

## Overview

Arches-HAL is a hardware abstraction and validation suite targeting **NVIDIA Jetson SoMs** on a Tegra-centric carrier, with an **STM32 co-processor** handling deterministic and real-time I/O. The public HAL API is preserved while backend implementations are retargeted to Tegra under JetPack/L4T and to the STM32 host transport for GPIO, PWM, and similar low-latency operations.

### Target Platform

| Property | Value |
|---|---|
| SoC | NVIDIA Tegra (Jetson Xavier / Orin family) |
| Carrier I/O | Tegra SoC interfaces plus external STM32 for deterministic I/O |
| GPU | Integrated NVIDIA Ampere / Volta-class Jetson GPU |
| Memory | Platform-dependent LPDDR4x / LPDDR5 |
| Real-time I/O | STM32 over UART, SPI, or USB-CDC |
| OS | JetPack / L4T |

### Key Features

- **Modular Library Architecture**: Separate libraries for CPU, GPU, DLA, video codec, camera, audio, PCIe, USB, GPIO, storage, Ethernet, and more.
- **Shared Platform Boundary**: Centralised constants in `src/platform_arches.h` and transport protocol support in `src/stm32_transport.*`.
- **STM32 Offload Contract**: Deterministic GPIO/PWM-style operations route through an explicit host-to-STM32 message protocol.
- **Cross-compilation via Docker**: ARM64 Ubuntu container for Tegra-oriented builds.
- **Comprehensive Testing**: Unit tests, integration tests, and deployment workflows.
- **Documentation Generation**: Integrated Doxygen support.
- **Code Quality Tools**: clang-format and cppcheck.

## Prerequisites

### System Requirements

- **Operating System**: Linux (Ubuntu 20.04+ recommended for host)
- **Compiler**: GCC 11+ or Clang 14+
- **Build Tools**:
  - CMake 3.10+
  - Ninja build system
  - Make
  - Git
- Docker installed and running (for cross-compilation)

## Build Process

The project supports two primary build methods:

1. **Native Compilation**: Build directly on a Jetson target running JetPack/L4T.
2. **Docker-based Cross-Compilation**: Build for ARM64 on any Linux host using the project container.

### Native Compilation (on Jetson)

#### Install Dependencies
```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build git \
                    protobuf-compiler libprotobuf-dev libprotoc-dev \
                    libgrpc++-dev libsqlite3-dev libboost-system-dev \
                    libboost-date-time-dev doxygen clang-format cppcheck
```

#### Build and Run Tests
```bash
make build
make run
```

### Docker-based Cross-Compilation

The Docker image is based on `ubuntu:22.04` (ARM64) and is named `arches-hal`.

#### Build Process

1. **Ensure Docker is Installed**
   ```bash
   make docker-install
   ```

2. **Build the Docker Image**
   ```bash
   make docker-image
   ```

   This creates a Docker image named `arches-hal` based on `ubuntu:22.04` with all necessary ARM64 build dependencies pre-installed.

3. **Build**
   ```bash
   make docker-build
   ```

   This command:
   - Runs the Docker container with the project mounted
   - Configures the build system with Ninja generator
   - Builds all targets
   - Outputs binaries in the `build/cross/` directory

4. **Deploy and Run on Target Jetson**

```bash
   # For Pi 5 Primary (192.168.1.3)
   make run-a
   # For Pi 5 Secondary (192.168.1.13)
   make run-b
   # Specify custom IP address
   make run-b IP_B=192.168.200.2
```

## Platform-Specific Notes

### AI Inference (DLA)
- **Hailo-8** (PCIe HAT+): detected at `/dev/hailo0`, kernel module `hailo_pci` (vendor `0x1e60`)
- **Coral USB Edge TPU**: detected via USB vendor `18d1` at `/dev/apex_0`

### GPU (VideoCore VII)
- devfreq sysfs: `/sys/class/devfreq/fde00000.gpu/`
- DRM: `/dev/dri/card0`, `/dev/dri/renderD128`
- Module: `v3d`

### Video Codec
- H.264 encode/decode: `bcm2835-codec` at `/dev/video10` (encoder), `/dev/video11` (decoder)
- H.265/HEVC decode: `rpivid` at `/dev/video18` or `/dev/video19`

### Camera (CSI)
- Unicam CSI-2 controller: `/sys/bus/platform/devices/1f00110000.csi`
- V4L2 device: `/dev/video0`, media controller: `/dev/media0`

### Debug UART
- `/dev/ttyAMA0` (115200 8N1 by default on Pi 5)

## Make Targets

For the full list of supported commands, run:

```bash
make help
```

## Contributing

1. Follow the established code formatting (use `make format`)
2. Run static analysis before submitting (`make cppcheck`)
3. Add tests for new functionality
4. Update documentation as needed





 git add . ;  git commit -am "Initial commit" ; git push origin --all

