---
description: Build and test Arches HAL for the ARM64 Jetson target using Docker/QEMU
allowed-tools: Bash(make docker-test)
---

Run the ARM64 cross-build and test suite:

!`make docker-test`

This command requires Docker, a usable ARM64/QEMU setup, and the local `arches`
image. Report unavailable dependencies as blocked; do not deploy to hardware or
attempt to install Docker automatically.