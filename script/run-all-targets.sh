#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# run-all-targets.sh — Run every Makefile target and generate a pass/fail report
#
# Usage:
#   ./script/run-all-targets.sh              # run all targets
#   ./script/run-all-targets.sh --quick      # skip slow targets (docker, deploy, run)
#   ./script/run-all-targets.sh --build      # only native build + test + package
#   ./script/run-all-targets.sh --list       # just list targets and exit
#
# The script runs each target once, captures stdout+stderr, and produces a
# colour-coded summary table at the end.  A target that depends on others
# (e.g. "build" depends on "configure") still runs its dependency chain;
# the report records only the named target's own exit code.
#
# Exit code: 0 if all targets pass, 1 if any fail.
# ---------------------------------------------------------------------------

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

RESULTS_DIR="${PROJECT_DIR}/.claude/reports/target-run"
mkdir -p "$RESULTS_DIR"

# ---------------------------------------------------------------------------
# Target groups — ordered so dependencies run first
# ---------------------------------------------------------------------------
ALL_TARGETS=(
  # Native builds
  configure
  build
  test
  run
  # Common
  clean
  list
  # Cross builds (Docker)
  docker-install
  docker-image
  docker-configure
  docker-build
  docker-test
  docker-clean
  docker-shell
  # Deploy & Run (remote)
  deploy
  deploy-a
  deploy-b
  run-a
  run-b
  # Packaging
  package-deb
  package-rpm
  package-ipk
  package
  docker-package-deb
  docker-package-rpm
  docker-package-ipk
  docker-package
  # Code quality & Docs
  format
  cppcheck
  doc
  # Misc
  setup-completion
  help
)

# Targets that are slow or require special hardware / network
SLOW_TARGETS=(
  test
  run
  docker-test
  docker-build
  docker-configure
  docker-package-deb
  docker-package-rpm
  docker-package-ipk
  docker-package
  deploy
  deploy-a
  deploy-b
  run-a
  run-b
)

BUILD_TARGETS=(
  configure
  build
  test
  clean
  list
  package-deb
  package-ipk
  package
  format
  cppcheck
  doc
  help
)

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
mode="all"
if [[ "${1:-}" == "--quick" ]]; then
  mode="quick"
elif [[ "${1:-}" == "--build" ]]; then
  mode="build"
elif [[ "${1:-}" == "--list" ]]; then
  echo "Available targets (in run order):"
  for t in "${ALL_TARGETS[@]}"; do
    echo "  $t"
  done
  exit 0
fi

case "$mode" in
  quick)
    TARGETS=()
    for t in "${ALL_TARGETS[@]}"; do
      skip=false
      for slow in "${SLOW_TARGETS[@]}"; do
        [[ "$t" == "$slow" ]] && { skip=true; break; }
      done
      $skip || TARGETS+=("$t")
    done
    ;;
  build)
    TARGETS=("${BUILD_TARGETS[@]}")
    ;;
  *)
    TARGETS=("${ALL_TARGETS[@]}")
    ;;
esac

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
pad() {
  local str="$1" width="$2"
  printf "%-${width}s" "$str"
}

run_target() {
  local target="$1" logfile="$RESULTS_DIR/${target//\//_}.log"
  local start end elapsed exit_code

  start=$(date +%s%N)
  set +e
  cd "$PROJECT_DIR"
  output=$(make "$target" 2>&1)
  exit_code=$?
  set -e
  end=$(date +%s%N)
  elapsed=$(( (end - start) / 1000000 ))  # ms

  echo "$output" > "$logfile"
  echo "$exit_code" > "${logfile%.log}.exit"

  # Format elapsed time
  if [ "$elapsed" -ge 60000 ]; then
    local sec=$((elapsed / 1000))
    local min=$((sec / 60))
    local remain_sec=$((sec % 60))
    elapsed_fmt="${min}m${remain_sec}s"
  elif [ "$elapsed" -ge 1000 ]; then
    elapsed_fmt="$(echo "scale=1; $elapsed/1000" | bc)s"
  else
    elapsed_fmt="${elapsed}ms"
  fi

  echo "$elapsed_fmt" > "${logfile%.log}.time"
  echo "$exit_code"
}

# ---------------------------------------------------------------------------
# Header
# ---------------------------------------------------------------------------
echo ""
echo -e "${BOLD}══════════════════════════════════════════════════════════════${NC}"
echo -e "${BOLD}  Arches-HAL — Makefile Target Report${NC}"
echo -e "${BOLD}  Mode: ${mode}${NC}"
echo -e "${BOLD}  $(date)${NC}"
echo -e "${BOLD}══════════════════════════════════════════════════════════════${NC}"
echo ""

# ---------------------------------------------------------------------------
# Run each target
# ---------------------------------------------------------------------------
declare -A RESULTS
declare -A ELAPSED

total=${#TARGETS[@]}
count=0
fail_count=0

for target in "${TARGETS[@]}"; do
  count=$((count + 1))
  printf "  [%2d/%2d] %-30s ... " "$count" "$total" "$target"

  exit_code=$(run_target "$target")
  elapsed_fmt=$(cat "$RESULTS_DIR/${target//\//_}.time")

  if [ "$exit_code" -eq 0 ]; then
    echo -e "${GREEN}PASS${NC}  (${elapsed_fmt})"
    RESULTS["$target"]="PASS"
  else
    echo -e "${RED}FAIL${NC}  (${elapsed_fmt})"
    RESULTS["$target"]="FAIL"
    fail_count=$((fail_count + 1))
  fi
  ELAPSED["$target"]="$elapsed_fmt"
done

# ---------------------------------------------------------------------------
# Summary table
# ---------------------------------------------------------------------------
echo ""
echo -e "${BOLD}──────────────────────────────────────────────────────────────${NC}"
echo -e "${BOLD}  Summary${NC}"
echo -e "${BOLD}──────────────────────────────────────────────────────────────${NC}"
echo ""

# Group targets by category
declare -A CATEGORIES
CATEGORIES["Native builds"]="configure build test run"
CATEGORIES["Common"]="clean list"
CATEGORIES["Cross builds (Docker)"]="docker-install docker-image docker-configure docker-build docker-test docker-clean docker-shell"
CATEGORIES["Deploy & Run"]="deploy deploy-a deploy-b run-a run-b"
CATEGORIES["Packaging"]="package-deb package-rpm package-ipk package docker-package-deb docker-package-rpm docker-package-ipk docker-package"
CATEGORIES["Code quality & Docs"]="format cppcheck doc"
CATEGORIES["Misc"]="setup-completion help"

for group in "Native builds" "Common" "Cross builds (Docker)" "Deploy & Run" "Packaging" "Code quality & Docs" "Misc"; do
  targets_in_group="${CATEGORIES[$group]}"
  # Only show groups that had targets run
  shown=false
  for t in $targets_in_group; do
    if [[ " ${TARGETS[*]} " == *" $t "* ]]; then
      if [ "$shown" = false ]; then
        echo -e "  ${BOLD}$group${NC}"
        shown=true
      fi
      result="${RESULTS[$t]:----}"
      elapsed="${ELAPSED[$t]:-}"
      if [ "$result" = "PASS" ]; then
        echo -e "    ${GREEN}✔${NC}  $(pad "$t" 28) ${GREEN}PASS${NC}  (${elapsed})"
      else
        echo -e "    ${RED}✘${NC}  $(pad "$t" 28) ${RED}FAIL${NC}  (${elapsed})"
      fi
    fi
  done
  [ "$shown" = true ] && echo ""
done

# ---------------------------------------------------------------------------
# Footer
# ---------------------------------------------------------------------------
pass_count=$((total - fail_count))
echo -e "${BOLD}──────────────────────────────────────────────────────────────${NC}"
if [ "$fail_count" -eq 0 ]; then
  echo -e "${GREEN}  All ${total} targets passed.${NC}"
else
  echo -e "${YELLOW}  ${pass_count}/${total} passed, ${RED}${fail_count} failed${NC}${YELLOW}.${NC}"
  echo ""
  echo -e "  ${BOLD}Failed targets:${NC}"
  for target in "${TARGETS[@]}"; do
    if [ "${RESULTS[$target]}" = "FAIL" ]; then
      echo -e "    ${RED}✘${NC}  $target  (log: ${RESULTS_DIR}/${target//\//_}.log)"
    fi
  done
fi
echo -e "${BOLD}──────────────────────────────────────────────────────────────${NC}"
echo ""
echo -e "  Logs: ${CYAN}${RESULTS_DIR}/${NC}"
echo ""

exit $fail_count
