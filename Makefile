# Master Makefile for Arches HAL
# Supports both native and Docker-based builds with Ninja

.DEFAULT_GOAL := help

.PHONY: all build configure clean test format cppcheck doc doc-pdf help
.PHONY: package package-deb package-rpm package-ipk
.PHONY: docker-build docker-configure docker-clean docker-shell docker-install docker-image docker-test
.PHONY: docker-package docker-package-deb docker-package-rpm docker-package-ipk
.PHONY: deploy deploy-a deploy-b run run-a run-b list setup-completion

# Build directories (separate to avoid CMake cache conflicts)
NATIVE_BUILD_DIR := build/native
CROSS_BUILD_DIR := build/cross

# Parallel jobs (override with make JOBS=N)
JOBS := $(shell nproc)
DOCKER_IMAGE := arches
DOCKER_CROSS_BUILD_DIR := /workspace/build/cross

# Host architecture as shown by file(1) (e.g. x86-64, aarch64)
HOST_ARCH := $(shell uname -m | sed 's/x86_64/x86-64/')

# IP addresses for Raspberry Pi 5 target devices
# Override on the command line: make deploy IP_A=<addr> IP_B=<addr>
IP_A := 192.168.1.3
IP_B := 192.168.1.13

# User and password for SSH/SCP.
# Override on the command line or via environment variables:
#   make deploy USER=pi PASS=<password>
#   HAL_SSH_USER=pi HAL_SSH_PASS=<password> make deploy
# Do NOT commit real credentials here.
USER := $(or $(HAL_SSH_USER),nvidia)
PASS := $(or $(HAL_SSH_PASS),nvidia)
SSHPASS := sshpass -p $(PASS)
SSH := $(SSHPASS) ssh -o StrictHostKeyChecking=no
SCP := $(SSHPASS) scp -o StrictHostKeyChecking=no

# Default target
all: docker-build ## Alias for docker-build (default)

# Deploy to remote target: $(1)=IP address
define deploy_remote
	@echo "Deploying to $(1)..."
	$(SSH) $(USER)@$(1) "rm -rf /tmp/acu-test && mkdir -p /tmp/acu-test"
	$(SCP) -r $(CROSS_BUILD_DIR)/lib $(USER)@$(1):/tmp/acu-test/
	$(SCP) -r $(CROSS_BUILD_DIR)/bin $(USER)@$(1):/tmp/acu-test/
endef

# Run apps on remote target: $(1)=IP address
define run_remote
	@APP="$(word 2, $(MAKECMDGOALS))"; \
	ARGS="$(wordlist 3, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))"; \
	if [ -z "$$APP" ]; then \
		echo "Running all test applications on $(1)..."; \
		for name in $$($(SSH) $(USER)@$(1) "ls /tmp/acu-test/bin/"); do \
			echo "--- Running $$name ---"; \
			$(SSH) $(USER)@$(1) "cd /tmp/acu-test && ./bin/$$name" || true; \
		done; \
	else \
		echo "Running $$APP on $(1) with args: $$ARGS"; \
		$(SSH) $(USER)@$(1) "cd /tmp/acu-test && ./bin/$$APP $$ARGS"; \
	fi
endef

define run_local
	@APP="$(word 2, $(MAKECMDGOALS))"; \
	ARGS="$(wordlist 3, $(words $(MAKECMDGOALS)), $(MAKECMDGOALS))"; \
	if [ -z "$$APP" ]; then \
		echo "Running all test applications locally..."; \
		for name in $$(ls $(NATIVE_BUILD_DIR)/bin/); do \
			echo "--- Running $$name ---"; \
			$(if $(V),echo "LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$name";) \
			LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$name || true; \
		done; \
	else \
		echo "Running $$APP locally with args: $$ARGS"; \
		$(if $(V),echo "LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$APP $$ARGS";) \
		LD_LIBRARY_PATH=$(NATIVE_BUILD_DIR)/lib $(NATIVE_BUILD_DIR)/bin/$$APP $$ARGS; \
	fi
endef

# =============================================================================
# Native builds (host architecture)
# =============================================================================

configure: ## Configure the project with CMake/Ninja
	@echo "Configuring project with Ninja..."
	mkdir -p $(NATIVE_BUILD_DIR)
	cd $(NATIVE_BUILD_DIR) && cmake -GNinja $(CURDIR)

build: configure ## Build the project (configures if needed)
	@echo "Building project with Ninja..."
	cd $(NATIVE_BUILD_DIR) && ninja -j$(JOBS)

run: build ## Run test app locally (usage: make run [name] [args...], no args = run all)
	$(call run_local)

test: build ## Run tests with CTest
	@echo "Running tests with CTest..."
	cd $(NATIVE_BUILD_DIR) && ctest --output-on-failure -j$(JOBS)

# =============================================================================
# Common for native and Cross
# =============================================================================

clean: ## Remove build directory
	@echo "Cleaning build directory..."
	rm -rf build

list: ## List available test applications
	@echo "Native ($(NATIVE_BUILD_DIR)):"
	@ls -1 $(NATIVE_BUILD_DIR)/bin/ 2>/dev/null | sed 's/^/    /' || echo "    (none - run 'make build' first)"
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
# Deploy & Run (on target device)
# =============================================================================

deploy: deploy-a deploy-b ## Deploy to all devices (deploy-a + deploy-b)

deploy-a: docker-build ## Deploy lib and bin to $(IP_A)
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		echo "Deploying to $(IP_A)..."; \
		$(SSH) $(USER)@$(IP_A) "rm -rf /tmp/acu-test && mkdir -p /tmp/acu-test"; \
		$(SCP) -r $(CROSS_BUILD_DIR)/lib $(USER)@$(IP_A):/tmp/acu-test/; \
		$(SCP) -r $(CROSS_BUILD_DIR)/bin $(USER)@$(IP_A):/tmp/acu-test/; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

deploy-b: docker-build ## Deploy lib and bin to $(IP_B)
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		echo "Deploying to $(IP_B)..."; \
		$(SSH) $(USER)@$(IP_B) "rm -rf /tmp/acu-test && mkdir -p /tmp/acu-test"; \
		$(SCP) -r $(CROSS_BUILD_DIR)/lib $(USER)@$(IP_B):/tmp/acu-test/; \
		$(SCP) -r $(CROSS_BUILD_DIR)/bin $(USER)@$(IP_B):/tmp/acu-test/; \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

run-a: deploy-a ## Run test app on $(IP_A) (usage: make run-a [name] [args...])
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(call run_remote,$(IP_A)); \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

run-b: deploy-b ## Run test app on $(IP_B) (usage: make run-b [name] [args...])
	@if [ "$(QEMU_AVAILABLE)" = "yes" ]; then \
		$(call run_remote,$(IP_B)); \
	else \
		echo "  QEMU user-static not available. Install with: sudo apt-get install qemu-user-static"; \
	fi

# =============================================================================
# Code quality & Docs
# =============================================================================

format: configure ## Format code with clang-format (and cmake-format if available)
	@echo "Formatting C++ files with clang-format..."
	@if command -v clang-format > /dev/null 2>&1; then \
		cd $(NATIVE_BUILD_DIR) && ninja format; \
	else \
		echo "  clang-format not found, skipping C++ formatting."; \
	fi
	@echo "Formatting CMake files..."
	@if command -v cmake-format > /dev/null 2>&1; then \
		find . -name CMakeLists.txt -not -path './build/*' | xargs cmake-format -i; \
	else \
		echo "  cmake-format not found, skipping CMake file formatting."; \
	fi

cppcheck: configure ## Run static analysis with cppcheck
	@echo "Running cppcheck static analysis..."
	cd $(NATIVE_BUILD_DIR) && ninja cppcheck

doc: configure ## Generate Doxygen documentation (HTML + XML + man)
	@echo "Generating documentation (HTML + XML + man)..."
	@if command -v doxygen > /dev/null 2>&1; then \
		cd $(NATIVE_BUILD_DIR) && ninja doc_doxygen; \
		echo "  HTML: $(NATIVE_BUILD_DIR)/docs/html/index.html"; \
	else \
		echo "  Doxygen not found. Run 'make install-tools' or apt-get install doxygen graphviz."; \
	fi

doc-pdf: configure ## Generate Doxygen documentation + build PDF reference (needs pdflatex)
	@echo "Generating documentation and building the PDF reference..."
	@if ! command -v doxygen > /dev/null 2>&1; then \
		echo "  Doxygen not found. Run 'make install-tools' or apt-get install doxygen graphviz."; \
	elif ! command -v pdflatex > /dev/null 2>&1; then \
		echo "  pdflatex not found. Run 'make install-tools' or apt-get install texlive-latex-extra."; \
	else \
		cd $(NATIVE_BUILD_DIR) && ninja doc_doxygen_pdf; \
		echo "  PDF: $(NATIVE_BUILD_DIR)/docs/Arches-HAL-Reference.pdf"; \
	fi

# =============================================================================
# Packaging (DEB / IPK / RPM)
# =============================================================================

package-deb: build ## Generate DEB package (native build)
	@echo "Generating DEB package..."
	cd $(NATIVE_BUILD_DIR) && cpack -G DEB
	@echo "Package: $$(ls $(NATIVE_BUILD_DIR)/arches-hal_*.deb 2>/dev/null | head -1)"

package-rpm: build ## Generate RPM package (requires rpmbuild)
	@echo "Generating RPM package..."
	@if command -v rpmbuild > /dev/null 2>&1; then \
		cd $(NATIVE_BUILD_DIR) && cpack -G RPM; \
		echo "Package: $$(ls $(NATIVE_BUILD_DIR)/arches-hal-*.rpm 2>/dev/null | head -1)"; \
	else \
		echo "  rpmbuild not found. Install with: sudo apt-get install rpm"; \
	fi

package-ipk: build ## Generate IPK package (DEB renamed .ipk)
	@echo "Generating IPK package..."
	cd $(NATIVE_BUILD_DIR) && cmake --build . --target package_ipk
	@echo "Package: $$(ls $(NATIVE_BUILD_DIR)/arches-hal_*.ipk 2>/dev/null | head -1)"

package: package-deb package-ipk ## Generate DEB + IPK (native build)
	@echo "All packages generated in $(NATIVE_BUILD_DIR)/"

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

run-all-targets: ## Run all targets and generate pass/fail report
	@echo "Running all Makefile targets and generating report..."
	@if [ -x script/run-all-targets.sh ]; then \
		script/run-all-targets.sh; \
	else \
		echo "  script/run-all-targets.sh not found. Run from the project root."; \
	fi

run-all-targets-quick: ## Skip slow/hardware targets
	@echo "Running all Makefile targets (quick mode) and generating report..."
	@if [ -x script/run-all-targets.sh ]; then \
		script/run-all-targets.sh --quick; \
	else \
		echo "  script/run-all-targets.sh not found. Run from the project root."; \
	fi

run-all-targets-build: ## Only native build + test + package
	@echo "Running build targets and generating report..."
	@if [ -x script/run-all-targets.sh ]; then \
		script/run-all-targets.sh --build; \
	else \
		echo "  script/run-all-targets.sh not found. Run from the project root."; \
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
