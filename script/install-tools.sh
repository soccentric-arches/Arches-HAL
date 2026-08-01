#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# install-tools.sh — Install all host tools required by Arches-HAL
#
# Usage:
#   ./script/install-tools.sh              # install everything
#   ./script/install-tools.sh --check       # only check what's missing
#   ./script/install-tools.sh --docker      # install Docker (not apt)
#
# Installs:
#   clang-format          C++ formatting (format target)
#   cmake-format          CMake file formatting (format target)
#   doxygen               API documentation generation (doc target)
#   graphviz              Call/caller/class graphs in Doxygen output (doc target)
#   texlive-latex-extra    LaTeX toolchain for the PDF reference (doc-pdf target)
#   rpm                   rpmbuild for RPM packaging (package-rpm target)
#   qemu-user-static      ARM64 binary emulation (docker-* targets)
#   sshpass               SSH password auth for deploy targets
# ---------------------------------------------------------------------------

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_pass() { echo -e "${GREEN}[PASS]${NC} $1"; }
log_fail() { echo -e "${RED}[FAIL]${NC} $1"; }
log_info() { echo -e "${YELLOW}[INFO]${NC} $1"; }

# ---------------------------------------------------------------------------
# Tool list: (binary name, apt package name, human-readable label)
# ---------------------------------------------------------------------------
TOOLS=(
  "clang-format:clang-format:C++ formatter"
  "cmake-format:cmake-format:CMake formatter"
  "doxygen:doxygen:API documentation generator"
  "dot:graphviz:Graphviz (Doxygen call/caller/class graphs)"
  "pdflatex:texlive-latex-extra:LaTeX toolchain (Doxygen PDF reference)"
  "rpmbuild:rpm:RPM package builder"
  "qemu-aarch64-static:qemu-user-static:ARM64 QEMU emulation"
  "sshpass:sshpass:SSH password auth"
)

# ---------------------------------------------------------------------------
# Check mode
# ---------------------------------------------------------------------------
check_only=false
if [[ "${1:-}" == "--check" ]]; then
  check_only=true
elif [[ "${1:-}" == "--docker" ]]; then
  log_info "Docker install mode not implemented via this script."
  log_info "See https://docs.docker.com/engine/install/ubuntu/ or run:"
  log_info "  sudo apt-get install docker.io"
  exit 0
fi

# ---------------------------------------------------------------------------
# Check / install each tool
# ---------------------------------------------------------------------------
missing=()
installed=()

for entry in "${TOOLS[@]}"; do
  IFS=':' read -r binary pkg label <<< "$entry"

  if command -v "$binary" >/dev/null 2>&1; then
    log_pass "$label ($binary)"
    installed+=("$binary")
  else
    missing+=("$pkg")
    log_fail "$label ($binary) — will install package '$pkg'"
  fi
done

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
if [ ${#missing[@]} -eq 0 ]; then
  echo ""
  log_info "All tools are already installed."
  exit 0
fi

if [ "$check_only" = true ]; then
  echo ""
  log_info "Missing packages: ${missing[*]}"
  exit 1
fi

# ---------------------------------------------------------------------------
# Pre-flight: ensure we can sudo (only needed for actual install)
# ---------------------------------------------------------------------------
if ! sudo -n true 2>/dev/null; then
  echo ""
  log_fail "sudo access is required to install packages."
  echo "  Run: sudo -v  (to cache credentials), then re-run this script."
  exit 1
fi

echo ""
log_info "Installing ${#missing[@]} missing package(s): ${missing[*]}"
echo ""

sudo apt-get update
sudo apt-get install -y "${missing[@]}"

echo ""

# ---------------------------------------------------------------------------
# Post-install: register QEMU binfmt for ARM64
# ---------------------------------------------------------------------------
if command -v qemu-aarch64-static >/dev/null 2>&1; then
  log_info "Registering QEMU binfmt for ARM64 emulation..."
  if ls /proc/sys/fs/binfmt_misc/qemu-aarch64 >/dev/null 2>&1; then
    log_pass "QEMU binfmt already registered for aarch64"
  else
    # Register via the multiarch helper container (preferred)
    if docker info >/dev/null 2>&1; then
      docker run --rm --privileged multiarch/qemu-user-static --reset -p -c 2>/dev/null && \
        log_pass "QEMU binfmt registered" || \
        log_fail "Could not register QEMU binfmt — try: sudo apt-get install qemu-user-static"
    else
      log_fail "Docker not available — cannot register QEMU binfmt automatically."
      log_info "After installing Docker, run: docker run --rm --privileged multiarch/qemu-user-static --reset -p -c"
    fi
  fi
fi

# ---------------------------------------------------------------------------
# Verify
# ---------------------------------------------------------------------------
echo ""
log_info "Verifying installation..."
all_ok=true
for entry in "${TOOLS[@]}"; do
  IFS=':' read -r binary pkg label <<< "$entry"
  if command -v "$binary" >/dev/null 2>&1; then
    log_pass "$label ($binary)"
  else
    log_fail "$label ($binary) — install failed"
    all_ok=false
  fi
done

echo ""
if [ "$all_ok" = true ]; then
  log_info "All tools installed successfully."
  log_info "Run 'make format' / 'make doc' / 'make doc-pdf' / 'make package-rpm' / 'make docker-build' to use them."
else
  log_fail "Some tools failed to install. Check errors above."
  exit 1
fi
