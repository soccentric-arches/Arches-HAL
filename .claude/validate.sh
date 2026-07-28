#!/usr/bin/env bash
# Arches-HAL — per-repo validation gate.  See .claude/lib/validate-lib.sh.
#
#   quick   seconds. Repo-shape + static checks. Safe as an edit-time hook.
#   full    minutes. Configure, build, and the full CTest set.
#
# HAL is the hub: it owns the canonical .proto contracts that Client and Server
# link against. The quick tier therefore guards the contract, not just the code.
set -uo pipefail
# shellcheck source=lib/validate-lib.sh
source "$(dirname "${BASH_SOURCE[0]}")/lib/validate-lib.sh"

TIER="$(tier_or_die "${1:-quick}")"

# --- quick -----------------------------------------------------------------

assert "proto contracts present" \
  'test -f protos/hal_service.proto && test -f protos/client_service.proto && test -f protos/common.proto'

# Generated code is committed here on purpose: Client and Server link HAL's
# generated symbols and must never regenerate protos themselves. A stray
# regeneration in this repo is how ODR/duplicate-symbol link errors start.
assert "no stray generated protos outside the tracked set" \
  '! find src -name "*.pb.cc" -newer protos/hal_service.proto -print -quit | grep -q .'

# The top-level protos are named one by one in protos/CMakeLists.txt (COMMON_PROTO,
# SERVICE_PROTO, CLIENT_SERVICE_PROTO); only modules/*.proto is globbed. So a new
# top-level .proto is silently left out of the build unless it is added there.
assert "every top-level proto is named in protos/CMakeLists.txt" \
  'for f in protos/*.proto; do
     grep -qF "$(basename "$f")" protos/CMakeLists.txt \
       || { echo "not referenced by protos/CMakeLists.txt: $f"; exit 1; }
   done'

assert "modules/*.proto is still glob-collected" \
  'grep -qE "file\(GLOB[^)]*modules/\*\.proto" protos/CMakeLists.txt'

check "cmake configure parses" "cmake ninja" \
  'cmake -GNinja -S . -B build/native >/dev/null'

# NOTE: HAL has no `make lint` target — unlike Client, Server, Configuration,
# HQ and OS. `make cppcheck` is the nearest equivalent and it is best-effort:
# it exits 0 when cppcheck is absent. We require the tool instead of inheriting
# that green exit.
check "cppcheck static analysis" "cppcheck cmake ninja" \
  'make cppcheck 2>&1 | tee /dev/stderr | grep -qv "cppcheck not installed"'

check "clang-format clean" "clang-format" \
  'find src protos -name "*.cc" -o -name "*.h" -o -name "*.cpp" 2>/dev/null \
     | xargs -r clang-format --dry-run --Werror'

[ "$TIER" = "quick" ] && { report "$TIER"; exit $?; }

# --- full ------------------------------------------------------------------

check "builds clean" "cmake ninja" 'make build'

# HAL's own `make test` runs the full ~36-executable set in HAL's build dir,
# which is correct here — the label filters exist for Client and Server, which
# link HAL and would otherwise trip over its unbuilt EXCLUDE_FROM_ALL targets.
check "full ctest suite" "cmake ninja ctest" 'make test'

report "$TIER"
