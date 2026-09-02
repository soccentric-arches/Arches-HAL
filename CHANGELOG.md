# Changelog

Maintained by the Loop Engine. One entry per loop that touched this repo,
newest first, appended on loop close (pass or fail) and committed on the loop's
own feature branch. See the master repo's `Arches-Loop-Orchestration-Spec-v2.md` §11.1.

Entries written by hand are fine; the engine inserts directly under this header.

## 2026-02-26 — Close the 16-module implementation gap

- Implemented all 16 modules that had proto contracts and registry entries but no
  native implementation or tests: i2c, spi, uart, rtc, battery, nvme, emmc,
  sd_card, can_bus, rs485, analog_input, pwm_output, bluetooth, mipi_csi,
  mipi_dsi, accelerometer. Each module follows the provider-injection pattern
  (SysfsProvider/CommandProvider injected; SKIPPED when hardware is absent) and
  ships with a test binary wired into CTest.
- Normalized the 16 pending entries in protos/registry/modules.yaml to the
  canonical schema and filled every tracking source_path/test_path (no nulls).
- Updated protos/README.md module count (35 → 51) and the README feature table.
- Removed a duplicated lib_base target block from src/CMakeLists.txt.
- Verified: native build clean under -Wall -Wextra -Wpedantic -Werror; full
  CTest suite passes 53/53.
