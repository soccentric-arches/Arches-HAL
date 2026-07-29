---
name: remote-test
description: "Use when: running Arches HAL test binaries on the real remote Jetson target devices (IP_A/IP_B) - deploying cross-built binaries over SSH and executing them on-device, or understanding how that differs from the QEMU cross-build ctest run."
---

# Run Tests On Remote Target Devices

There are **two different "remote" test paths** in this repo — do not conflate
them:

1. **`make docker-test`** — runs `ctest` for the cross/ARM64 build tree
   (`build/cross`), but *inside the Docker container on the dev machine* via
   QEMU emulation. No physical hardware involved. Fast to iterate, and a good
   first gate for ARM64/target-arch-specific bugs.
2. **`make deploy-*` / `make run-*`** — cross-builds, then copies binaries to
   an *actual* Jetson device over SSH and executes them there directly. This
   is the real end-to-end hardware verification and the subject of this
   skill. It is **not** CTest on the device — there is no aggregated
   pass/fail report; each binary's own exit status/output is what you read.

## Targets (from the top-level Makefile)

```bash
make deploy         # deploy-a + deploy-b
make deploy-a       # docker-build, then scp lib/ and bin/ to $(IP_A)
make deploy-b       # docker-build, then scp lib/ and bin/ to $(IP_B)
make run-a          # deploy-a, then run test app(s) on $(IP_A)
make run-b          # deploy-b, then run test app(s) on $(IP_B)
make run-a <name> [args...]   # run one specific binary with args on IP_A
make run-b <name> [args...]   # same, on IP_B
```

`run-a`/`run-b` with **no** `<name>` iterates every binary under
`/tmp/acu-test/bin/` on the device, running each in turn and continuing past
individual failures (best-effort `|| true`) — useful for a full device sweep,
but read the per-app output, since there is no final ctest-style summary.

## Devices, paths, credentials

- Target IPs: `IP_A` / `IP_B`, default `192.168.1.3` / `192.168.1.13`.
  Override: `make deploy IP_A=<addr> IP_B=<addr>`.
- SSH user/pass: `HAL_SSH_USER` (default `nvidia`) / `HAL_SSH_PASS` (default
  `nvidia`), via `sshpass`. Override via env vars, never hardcode real
  credentials into the Makefile:
  ```bash
  HAL_SSH_USER=pi HAL_SSH_PASS=<password> make deploy
  ```
- Remote path is `/tmp/acu-test/{lib,bin}` — a scratch directory, **wiped on
  every deploy** (`rm -rf /tmp/acu-test && mkdir -p ...`), not an install
  location. Don't rely on anything else persisting there between deploys.
- Requires `sshpass` on the machine running `make` (not on the target).

## Prerequisites

- Cross build must succeed first — `deploy-a`/`deploy-b` already depend on
  `docker-build`, so this happens automatically, but a broken cross build
  will surface here as a deploy/copy failure, not a test failure. If unsure,
  build the [build](../build/SKILL.md) skill's `make docker-build` first
  and confirm it's clean before deploying.
- Network reachability / SSH access to `IP_A`/`IP_B` from the dev machine.

## Recommended order

1. `make docker-test` — cheap QEMU-based sanity check, no device time spent.
2. `make run-a` / `make run-b` (or `make deploy` + manual per-binary runs) —
   confirm on real hardware, especially for anything touching STM32
   transport (`mcu_interface_lib`, `stm32_transport`), GPIO, or other
   physically-backed modules that QEMU cannot meaningfully exercise.
3. Read per-binary output directly — there's no CTest report to fall back on
   for the on-device run; a binary's own `hal::Status` output is the signal.
