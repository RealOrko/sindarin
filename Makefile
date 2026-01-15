# Sn Compiler - Makefile (CMake Wrapper)
#
# This Makefile provides familiar Make targets that delegate to CMake.
# For advanced usage, use CMake directly with presets:
#   cmake --preset linux-gcc-release
#   cmake --build --preset linux-gcc-release
#
# See CMakePresets.json for all available presets.

#------------------------------------------------------------------------------
# Phony targets
#------------------------------------------------------------------------------
.PHONY: all build rebuild run clean test help
.PHONY: test-unit test-integration test-integration-errors
.PHONY: test-explore test-explore-errors test-sdk
.PHONY: configure install package
.PHONY: setup-deps

#------------------------------------------------------------------------------
# Platform Detection
#------------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
    CMAKE_PRESET := windows-clang-release
    CMAKE_DEBUG_PRESET := windows-clang-debug
    EXE_EXT := .exe
    PYTHON := python
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifneq ($(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)),)
        PLATFORM := windows
        CMAKE_PRESET := windows-clang-release
        CMAKE_DEBUG_PRESET := windows-clang-debug
        EXE_EXT := .exe
        PYTHON := python
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM := darwin
        CMAKE_PRESET := macos-clang-release
        CMAKE_DEBUG_PRESET := macos-clang-debug
        EXE_EXT :=
        PYTHON := python3
    else
        PLATFORM := linux
        CMAKE_PRESET := linux-gcc-release
        CMAKE_DEBUG_PRESET := linux-gcc-debug
        EXE_EXT :=
        PYTHON := python3
    endif
endif

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------
BUILD_DIR := build
BIN_DIR := bin
SN := $(BIN_DIR)/sn$(EXE_EXT)

# Allow preset override
PRESET ?= $(CMAKE_PRESET)

# Colors
BOLD := \033[1m
GREEN := \033[0;32m
YELLOW := \033[0;33m
NC := \033[0m

#------------------------------------------------------------------------------
# Default target
#------------------------------------------------------------------------------
all: build

#------------------------------------------------------------------------------
# build - Configure and build the compiler
#------------------------------------------------------------------------------
build:
	@echo "$(BOLD)Building Sindarin compiler...$(NC)"
	@echo "Platform: $(PLATFORM)"
	@echo "Preset: $(PRESET)"
	@# Check for stale CMake cache (different source directory)
	@if [ -f "$(BUILD_DIR)/CMakeCache.txt" ]; then \
		CACHED_DIR=$$(grep "CMAKE_HOME_DIRECTORY:INTERNAL=" $(BUILD_DIR)/CMakeCache.txt 2>/dev/null | cut -d= -f2); \
		if [ -n "$$CACHED_DIR" ] && [ "$$CACHED_DIR" != "$$(pwd)" ]; then \
			echo "$(YELLOW)Detected stale CMake cache (was: $$CACHED_DIR)$(NC)"; \
			echo "Cleaning build directory..."; \
			rm -rf $(BUILD_DIR); \
		fi; \
	fi
	@if command -v ninja >/dev/null 2>&1; then \
		cmake -S . -B $(BUILD_DIR) -G Ninja \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_C_COMPILER=$(if $(filter windows,$(PLATFORM)),clang,$(if $(filter darwin,$(PLATFORM)),clang,gcc)); \
	else \
		echo "Ninja not found, using Unix Makefiles"; \
		cmake -S . -B $(BUILD_DIR) -G "Unix Makefiles" \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_C_COMPILER=$(if $(filter windows,$(PLATFORM)),clang,$(if $(filter darwin,$(PLATFORM)),clang,gcc)); \
	fi
	@cmake --build $(BUILD_DIR)
	@echo ""
	@echo "$(GREEN)Build complete!$(NC)"
	@echo "Compiler: $(SN)"

#------------------------------------------------------------------------------
# rebuild - Clean and build
#------------------------------------------------------------------------------
rebuild: clean build

#------------------------------------------------------------------------------
# configure - Just configure CMake (useful for IDE integration)
#------------------------------------------------------------------------------
configure:
	@echo "Configuring with preset: $(PRESET)"
	cmake --preset $(PRESET)

#------------------------------------------------------------------------------
# clean - Remove build artifacts
#------------------------------------------------------------------------------
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(BIN_DIR)/sn$(EXE_EXT) $(BIN_DIR)/tests$(EXE_EXT)
	@rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*.d
	@rm -rf $(BIN_DIR)/lib
	@echo "Clean complete."

#------------------------------------------------------------------------------
# run - Compile and run samples/main.sn
#------------------------------------------------------------------------------
run: build
	@echo "Running samples/main.sn..."
	@timeout 5s $(SN) samples/main.sn -o /tmp/hello-world$(EXE_EXT) -l 3 2>&1 || $(SN) samples/main.sn -o /tmp/hello-world$(EXE_EXT) -l 3
	@timeout 5s /tmp/hello-world$(EXE_EXT) || /tmp/hello-world$(EXE_EXT)

#------------------------------------------------------------------------------
# Test targets - Delegate to Python test runner
#------------------------------------------------------------------------------
test: build
	@echo "$(BOLD)Running all tests...$(NC)"
	@$(PYTHON) scripts/run_tests.py all --verbose

test-unit: build
	@$(PYTHON) scripts/run_tests.py unit --verbose

test-integration: build
	@$(PYTHON) scripts/run_tests.py integration --verbose

test-integration-errors: build
	@$(PYTHON) scripts/run_tests.py integration-errors --verbose

test-explore: build
	@$(PYTHON) scripts/run_tests.py explore --verbose

test-explore-errors: build
	@$(PYTHON) scripts/run_tests.py explore-errors --verbose

test-sdk: build
	@$(PYTHON) scripts/run_tests.py sdk --verbose

#------------------------------------------------------------------------------
# install - Install to system
#------------------------------------------------------------------------------
install: build
	@echo "Installing Sindarin compiler..."
	@cmake --install $(BUILD_DIR)

#------------------------------------------------------------------------------
# package - Create distributable packages
#------------------------------------------------------------------------------
package: build
	@echo "Creating packages..."
	@cd $(BUILD_DIR) && cpack

#------------------------------------------------------------------------------
# setup-deps - Install build dependencies
#------------------------------------------------------------------------------
setup-deps:
	@echo "Setting up build dependencies..."
	@$(PYTHON) scripts/setup_deps.py

#------------------------------------------------------------------------------
# help - Show available targets
#------------------------------------------------------------------------------
help:
	@echo "$(BOLD)Sindarin Compiler - Build System$(NC)"
	@echo ""
	@echo "$(BOLD)Quick Start:$(NC)"
	@echo "  make build        Build the compiler"
	@echo "  make test         Run all tests"
	@echo "  make run          Compile and run samples/main.sn"
	@echo ""
	@echo "$(BOLD)Build Targets:$(NC)"
	@echo "  make build        Build compiler (auto-detects platform)"
	@echo "  make rebuild      Clean and build"
	@echo "  make configure    Configure CMake only"
	@echo "  make clean        Remove build artifacts"
	@echo ""
	@echo "$(BOLD)Test Targets:$(NC)"
	@echo "  make test                   Run all tests"
	@echo "  make test-unit              Run unit tests only"
	@echo "  make test-integration       Run integration tests"
	@echo "  make test-integration-errors Run integration error tests"
	@echo "  make test-explore           Run exploratory tests"
	@echo "  make test-explore-errors    Run exploratory error tests"
	@echo "  make test-sdk               Run SDK tests"
	@echo ""
	@echo "$(BOLD)Distribution Targets:$(NC)"
	@echo "  make install      Install to system"
	@echo "  make package      Create distributable packages"
	@echo ""
	@echo "$(BOLD)Setup:$(NC)"
	@echo "  make setup-deps   Install build dependencies"
	@echo ""
	@echo "$(BOLD)CMake Presets (Advanced):$(NC)"
	@echo "  cmake --preset linux-gcc-release    Linux with GCC"
	@echo "  cmake --preset linux-clang-release  Linux with Clang"
	@echo "  cmake --preset windows-clang-release Windows with Clang"
	@echo "  cmake --preset macos-clang-release  macOS with Clang"
	@echo ""
	@echo "  Then: cmake --build --preset <preset-name>"
	@echo ""
	@echo "$(BOLD)Environment Variables:$(NC)"
	@echo "  PRESET=<name>     Override CMake preset"
	@echo "  SN_CC=<compiler>  C compiler for generated code"
	@echo "  SN_CFLAGS=<flags> Extra compiler flags"
	@echo "  SN_LDFLAGS=<flags> Extra linker flags"
	@echo ""
	@echo "$(BOLD)Platform:$(NC) $(PLATFORM)"
	@echo "$(BOLD)Preset:$(NC) $(PRESET)"
