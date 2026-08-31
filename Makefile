# Master Makefile for Arches HAL
# Docker-based builds: the default (and only) build path is Docker.

.DEFAULT_GOAL := help

.PHONY: all build clean test format cppcheck doc doc-pdf help
.PHONY: package package-deb package-rpm package-ipk
.PHONY: docker docker-build docker-configure docker-clean docker-shell docker-install docker-image docker-test
.PHONY: docker-package docker-package-deb docker-package-rpm docker-package-ipk
.PHONY: list setup-completion

# Build directories (separate to avoid CMake cache conflicts)
CROSS_BUILD_DIR := build/cross

# Parallel jobs (override with make JOBS=N)
JOBS := $(shell nproc)
DOCKER_IMAGE := arches
DOCKER_CROSS_BUILD_DIR := /workspace/build/cross

# Host architecture as shown by file(1) (e.g. x86-64, aarch64)
HOST_ARCH := $(shell uname -m | sed 's/x86_64/x86-64/')

# Default target
all: docker-build ## Default target: Docker cross-build (fleet standard)

# =============================================================================
# Build & test (Docker — the default build path)
# =============================================================================

build: docker-build ## Build the project in Docker (alias for docker-build)
test: docker-test ## Run tests in Docker (ARM64 via QEMU)

# =============================================================================
# Common for native and Cross
# =============================================================================

clean: ## Remove build directory
	@echo "Cleaning build directory..."
	rm -rf build

list: ## List available test applications
	@echo "Cross ($(CROSS_BUILD_DIR)):"
	@ls -1 $(CROSS_BUILD_DIR)/bin/ 2>/dev/null | sed 's/^/    /' || echo "    (none - run 'make docker-build' first)"

# =============================================================================
# Cross builds (Docker)
# =============================================================================

# -t: allocate a pseudo-TTY so Ctrl-C (SIGINT) is forwarded to the container
# --init: use tini as PID 1 to properly propagate signals to child processes (e.g. ninja)
# ccache volume: persist compilation cache across docker runs
# QEMU check for ARM64 emulation (check binfmt interpreter exists)
QEMU_AVAILABLE := $(shell test -f /usr/bin/qemu-aarch64-static && echo yes || echo no)

DOCKER_RUN := docker run --rm -t --init --platform linux/arm64 -v $(PWD):/workspace -v ccache-arches:/home/arches/.ccache $(DOCKER_IMAGE) bash -c

docker-install: ## Install Docker if not present
	@if ! command -v docker > /dev/null 2>&1; then \
		echo "Docker not found. Installing Docker..."; \
		sudo apt update && sudo apt install -y docker.io; \
	else \
		echo "Docker is already installed."; \
	fi

docker-image: docker-install ## Build Docker image if not exists
	@if ! docker image inspect $(DOCKER_IMAGE) >/dev/null 2>&1; then \
		echo "Building Docker image $(DOCKER_IMAGE)..."; \
		make -C docker build; \
	else \
		echo "Docker image $(DOCKER_IMAGE) already exists."; \
	fi

docker-configure: docker-image ## Configure project with Docker/Ninja
	@echo "Configuring project with Docker/Ninja..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(DOCKER_RUN) "mkdir -p $(DOCKER_CROSS_BUILD_DIR) && cd $(DOCKER_CROSS_BUILD_DIR) && cmake -GNinja /workspace"; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker: docker-build ## Single-command Docker cross-build (alias for docker-build)

docker-build: docker-image ## Build project with Docker/Ninja
	@echo "Building project with Docker/Ninja..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(DOCKER_RUN) "if [ ! -f $(DOCKER_CROSS_BUILD_DIR)/build.ninja ]; then mkdir -p $(DOCKER_CROSS_BUILD_DIR) && cd $(DOCKER_CROSS_BUILD_DIR) && cmake -GNinja /workspace; fi && cd $(DOCKER_CROSS_BUILD_DIR) && ninja -j$(JOBS)"; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker-clean: docker-image ## Clean Docker build artifacts
	@echo "Cleaning Docker build directory..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(DOCKER_RUN) "rm -rf $(DOCKER_CROSS_BUILD_DIR)"; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker-shell: docker-image ## Start Docker shell for development
	@echo "Starting Docker shell for development..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		docker run -it --rm --platform linux/arm64 -v $(PWD):/workspace -v ccache-arches:/home/arches/.ccache $(DOCKER_IMAGE) bash; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker-test: docker-build ## Run CTest inside Docker (ARM64 via QEMU)
	@echo "Running tests inside Docker (ARM64 via QEMU)..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ctest --output-on-failure -j$(JOBS)"; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker-package-deb: docker-build ## Generate DEB package (cross/ARM64)
	@echo "Generating DEB package (cross/ARM64)..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && cpack -G DEB"; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker-package-rpm: docker-build ## Generate RPM package (cross/ARM64)
	@echo "Generating RPM package (cross/ARM64, requires rpmbuild in image)..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && cpack -G RPM"; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker-package-ipk: docker-build ## Generate IPK package (cross/ARM64)
	@echo "Generating IPK package (cross/ARM64)..."
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && cmake --build . --target package_ipk"; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

docker-package: docker-package-deb docker-package-ipk ## Generate DEB + IPK (cross/ARM64)
	@echo "Cross packages generated in build/cross/"

# =============================================================================
# Code quality & Docs (run inside the Docker build container)
# =============================================================================

format: docker-image ## Format code with clang-format (in Docker)
	@echo "Running clang-format in Docker..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ninja format"

cppcheck: docker-image ## Run static analysis with cppcheck (in Docker)
	@echo "Running cppcheck in Docker..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ninja cppcheck"

doc: docker-image ## Generate Doxygen documentation (in Docker)
	@echo "Generating documentation in Docker..."
	$(DOCKER_RUN) "cd $(DOCKER_CROSS_BUILD_DIR) && ninja doc_doxygen"

# Packaging: the docker-package* targets above cover all packaging (Docker).

# =============================================================================
# Misc
# =============================================================================

setup-completion:
	@echo "Setting up bash completion for make..."
	@if ! grep -q "source $(PWD)/make_completion.bash" ~/.bashrc; then \
		echo "source $(PWD)/make_completion.bash" >> ~/.bashrc; \
		echo "Added completion to ~/.bashrc. Restart your shell or run 'source ~/.bashrc' to activate."; \
	else \
		echo "Completion already set up in ~/.bashrc."; \
	fi

install-tools: ## Install missing host tools (clang-format, doxygen, rpm, QEMU, ...)
	@echo "Installing missing host tools..."
	@if [ -x script/install-tools.sh ]; then \
		script/install-tools.sh; \
	else \
		echo "  script/install-tools.sh not found. Run from the project root."; \
	fi

# =============================================================================
# Help
# =============================================================================

# ── Fleet-standard aliases ────────────────────────────────────────────────────
# Same verbs mean the same thing in every repo; these forward to this
# repo's own targets rather than duplicating their recipes.
.PHONY: fmt lint
fmt: format ## Alias for format
lint: cppcheck ## Alias for cppcheck

# ── Help ──────────────────────────────────────────────────────────────────────
# Targets are self-documenting: text after `## ` on a target line is picked up
# by `make help` automatically, so the listing can never drift from the rules.
.PHONY: help
help: ## Show this help
	@printf '\n  \033[1m%s\033[0m\n' 'Arches-HAL'
	@printf '  Usage: make <target> [VAR=value ...]\n'
	@printf '\n'
	@awk 'BEGIN {FS = ":.*?## "} /^[a-zA-Z0-9_.\/-]+:.*?## / { printf "    \033[36m%-24s\033[0m %s\n", $$1, $$2 }' $(MAKEFILE_LIST)
	@printf '\n'
