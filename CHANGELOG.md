# Changelog

Maintained by the Loop Engine. One entry per loop that touched this repo,
newest first, appended on loop close (pass or fail) and committed on the loop's
own feature branch. See the master repo's `Arches-Loop-Orchestration-Spec-v2.md` §11.1.

Entries written by hand are fine; the engine inserts directly under this header.

## 2026-09-04 — Codebase-wide review and hardening pass

- **Test-runner integrity**: added opt-in `--strict` flag to `runTestApp`; exits 1
  when any test FAIL/ERRORs (SKIPPED never fails, so no-hardware CI keeps exit 0).
  All five modes honor it; documented in usage text.
- **Vacuous checks made real**: DLA element-wise ADD/MUL/MAX, ReLU/LeakyReLU/Sigmoid
  and batch-norm tests compared a value against the identical recomputed expression
  (could never fail); replaced with hard-coded expected vectors. Removed dead
  `currentClk > 0` block.
- **Absent-hardware honesty**: no-hardware paths that reported PASS now report
  SKIPPED — camera_interfaces (7 format/AE tests + CSI-2 lane aggregator),
  audio_interfaces testMultiChannelAudio (now parses actual `channels:` from
  hw_params instead of comparing two hard-coded constants),
  communication_interface SPI mode test early returns. RP1 residue renamed to
  platform-neutral keys in thermal_management.
- **Copy-paste bug**: duplicated `isModuleLoaded("unicam")` condition → second is
  now `bcm2835_unicam`.
- **Command-injection hardening**: new `isSafeName()` helper in module_helpers.h;
  validated names before interpolation in can_bus, mipi_dsi, spi, uart (2 sites).
- **Parse robustness**: exception-safe `parseDoubleOr`/`parseIntOr` helpers;
  converted ~20 unguarded `std::stod/stoi` sites across 11 modules (malformed
  sysfs no longer surfaces as CRITICAL ERROR).
- **Severity consistency**: FAIL flips in accelerometer, analog_input, battery,
  emmc now set ERROR/WARNING severity instead of leaving INFO.
- **Type fidelity**: raw integer readings (accel axes, ADC samples, eMMC life
  time, pre-EOL) stored as int64_t metrics instead of double.
- **STM32 transport**: `encodeFrame` now rejects payloads > 255 bytes instead of
  silently truncating; non-mock roundTrip reports "host-side protocol
  self-verification" instead of implying real MCU I/O.
- **Core fixes**: removed stale `platform_rpi5.h` include from lib_base.cpp;
  `runTests()` now rejects LONG_TERM/EVENT_BASED loudly; "Pi5 hw monitors" CLI
  text renamed to Arches.
- **Fork-lineage cleanup**: 16 files with `@copyright (C) Lucid Motors` →
  Soccentric LLC; repaired 8 headers whose includes were trapped inside the
  opening doc comment; removed duplicate includes in gnss/imu headers.
- Verified: native build clean (-Wall -Wextra -Wpedantic -Werror), CTest 53/53,
  validation gate `quick` 7/7 PASS, cppcheck knownCondition findings in touched
  files resolved.

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
