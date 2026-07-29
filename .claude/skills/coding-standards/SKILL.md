---
name: coding-standards
description: "Use when: enforcing or extending Arches HAL's coding standard - clang-format style, cppcheck static analysis, compiler warnings-as-errors, or adding/strengthening MISRA C++ compliance checks for safety-relevant modules."
---

# Coding Standards, Lint & MISRA Compliance

Arches HAL is C++17, built with -Wall -Wextra -Wpedantic -Werror
(CMakeLists.txt) — any compiler warning is a build failure, not just a
lint note. Treat new warnings the same as new compile errors.

## What's already wired up (CMakeLists.txt, ninja targets)

| Target | Tool | What it does |
|---|---|---|
| `ninja format` (`make format`) | `clang-format` | In-place format of every `src/*.cpp`, `src/*.h`, `tests/*.cpp`, `tests/*.h` |
| `ninja cppcheck` (`make cppcheck`) | `cppcheck` | `--enable=all --std=c++17 --language=c++ --platform=unix64 --error-exitcode=1 --inline-suppr`, suppressing `missingInclude`/`missingIncludeSystem` |
| build itself | GCC/Clang | `-Wall -Wextra -Wpedantic -Werror` |

Both format and cppcheck targets are conditional on find_program(...) —
they silently don't exist if the tool isn't installed. .claude/validate.sh
does not inherit that silence: its check helper treats a missing tool as
exit 77 (BLOCKED), never a pass (see the [validate](../validate/SKILL.md)
skill). Always validate through ./.claude/validate.sh quick, not just make
cppcheck/make format directly, so a missing toolchain is caught rather than
silently skipped.

### No .clang-format config file exists yet

Unlike some sibling repos in the fleet (e.g. Acadia-HAL), this repo has no
.clang-format at the root, so clang-format -i falls back to its built-in
default style (LLVM). If a project-specific style is ever needed, add a
.clang-format at the repo root — ninja format will pick it up
automatically with no CMake changes required. Don't add one speculatively;
only do it if asked, since it will reformat all 35 module trees at once.

### Suppressing a specific cppcheck finding

Prefer an inline suppression at the exact line over broadening
--suppress=... in CMakeLists.txt (which is repo-wide):

```cpp
// cppcheck-suppress <checkId>
```

Only widen the global suppress list for a check that's a genuine false
positive across the whole tree — missingInclude/missingIncludeSystem are
already suppressed for that reason (cppcheck can't see the cross-compilation
sysroot's headers).

## MISRA C++ compliance

There is no MISRA ruleset wired into this repo today. This matters most
for functional_safety_lib and reliability_lib (the two modules that
directly test safety-relevant behavior), and for anything reachable from
stm32_transport/mcu_interface_lib given it's a real-time safety-adjacent
link. If asked to add or strengthen MISRA compliance, know the constraints
before reaching for tooling:

- MISRA rule text is copyrighted by HORIBA MIRA — do not paste, generate,
  transcribe, or embed the rule descriptions/wording anywhere in this repo
  (source comments, scripts, docs). Only rule numbers and your own
  restated rationale are fine to reference.
- cppcheck ships a MISRA addon that works today without any copyrighted
  text — --addon=misra reports violations by rule number using cppcheck's
  own (non-MISRA-text) analysis. Enable it by adding --addon=misra to the
  existing cppcheck custom target in CMakeLists.txt, or run standalone:
  ```bash
  cppcheck --addon=misra --enable=all --std=c++17 --language=c++ src/ tests/
  ```
- Optional rule-text mapping: cppcheck's MISRA addon can print the actual
  rule text in its output if given a misra_rules.txt you supply yourself
  (from a licensed MISRA copy) via --rule-texts=<path>. Never commit that
  file to this repo (copyright) — keep it local/out-of-tree
  (.gitignore'd) if a developer wants that convenience.
- clang-tidy has no direct "MISRA" checker, but cppcoreguidelines-*,
  bugprone-*, and cert-* check families overlap heavily with MISRA C++
  intent (no raw owning pointers, no C-style casts, initialization rules,
  etc.) and are copyright-clean to enable. There's no .clang-tidy config in
  this repo yet — adding one is the lowest-friction way to raise the bar
  incrementally without a formal MISRA tool.
- Formal MISRA certification (if ever required for functional safety
  sign-off) needs a commercial tool with a licensed ruleset — e.g. PC-lint
  Plus, Parasoft C/C++test, or LDRA. That's a build/tooling decision for a
  human to make (new paid dependency, CI implications) — don't silently
  introduce one.

## Validating a coding-standard change

```bash
./.claude/validate.sh quick   # includes clang-format --dry-run --Werror and make cppcheck
```

Run this after any lint/format/MISRA-tooling change, and again after
formatting a large tree, since --dry-run --Werror is what actually gates
the change — a clean make format run doesn't by itself prove the tree is
compliant if the tool was silently absent.
