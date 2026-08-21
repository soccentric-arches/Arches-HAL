# Arches-HAL Build and Deployment

## 1. Purpose and scope

Arches-HAL is the C++ hardware-abstraction and protocol test library. It uses CMake/Ninja, builds natively on the host, and provides an ARM64 Docker/QEMU cross-build for NVIDIA Jetson-style targets. The `protos/` tree contains generated/shared protocol build inputs.

## 2. Prerequisites

Install a C++17 toolchain, CMake, Ninja, CTest, CPack, and the required protobuf/gRPC and project dependencies. For cross builds install Docker and ARM64 QEMU user emulation (`qemu-user-static`). SSH/SCP and `sshpass` are needed for target deployment. `script/install-tools.sh` can install repository development tools where supported.

## 3. Setup

From the repository root, native configuration is created by `make configure`. Use `make list` to see built test applications. For the cross path, use `make docker-install` and `make docker-image`; the Docker workflow mounts this repository and uses a persistent ccache volume.

## 4. Build and compile

- `make build` configures and builds the native Ninja tree in `build/native`; `make` defaults to the Docker build.
- `make docker-configure` configures the ARM64 tree.
- `make docker-build` builds `build/cross` through Docker/QEMU.
- `make list` lists native and cross test binaries.

## 5. Test and lint

- `make test` runs native CTest with `--output-on-failure`.
- `make docker-test` runs the native test suite in the ARM64 Docker environment when QEMU is available.
- `make cppcheck` runs best-effort static analysis.
- `make format` runs the Ninja format target and `cmake-format` if installed.
- `make doc` and `make doc-pdf` generate documentation when their configured tools are available.

## 6. Packaging and artifacts

`make package` creates CPack output; `make package-deb`, `make package-rpm`, and `make package-ipk` request the individual package formats. Native binaries/libraries are under `build/native`; cross artifacts are under `build/cross`.

## 7. Deployment, install, and run

- `make deploy-a` and `make deploy-b` copy cross-built `lib` and `bin` trees to the configured remote devices.
- `make deploy` runs both deployments.
- `make run` runs test applications locally; `make run-a`/`make run-b` run them remotely after deployment.
- Override `IP_A`, `IP_B`, `USER`, and `PASS`, or use the documented `HAL_SSH_USER` and `HAL_SSH_PASS` environment variables. Never commit real passwords.

These targets require reachable hardware and credentials. `make docker-shell` provides an interactive cross-build environment; it is not a target installation.

## 8. Cleanup

`make clean` removes native and cross build directories. `make docker-clean` removes the Docker cross build directory. Package files outside those directories should be archived or removed separately according to release policy.

## 9. Hardware, cloud, and manual verification caveats

Native CTest does not prove the board backends, device permissions, timing, or transport behavior. ARM64 Docker tests provide architecture coverage but still do not replace `make run-a`/`make run-b` on actual hardware. No cloud deployment target is defined.
