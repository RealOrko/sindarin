# Sn Compiler - Root Makefile
# Consolidates all build and test operations

#------------------------------------------------------------------------------
# Phony targets (grouped by category)
#------------------------------------------------------------------------------
.PHONY: all build rebuild run clean
.PHONY: build-gcc build-clang build-tcc
.PHONY: test test-unit test-integration test-integration-errors
.PHONY: test-explore test-explore-errors test-sdk test-threading
.PHONY: test-gcc test-clang test-tcc
.PHONY: assembly measure-optimization benchmark help

#------------------------------------------------------------------------------
# Platform Detection
#------------------------------------------------------------------------------
# Detect Windows (MSYS2/MinGW/Cygwin)
ifeq ($(OS),Windows_NT)
    PLATFORM := windows
else
    UNAME_S := $(shell uname -s 2>/dev/null || echo Unknown)
    ifneq ($(filter MINGW% MSYS% CYGWIN%,$(UNAME_S)),)
        PLATFORM := windows
    else ifeq ($(UNAME_S),Linux)
        PLATFORM := linux
    else ifeq ($(UNAME_S),Darwin)
        PLATFORM := darwin
    else
        PLATFORM := unknown
    endif
endif

# Platform-specific settings
ifeq ($(PLATFORM),windows)
    EXE_EXT := .exe
    # Use copy instead of symlink on Windows (symlinks require admin privileges)
    CREATE_SYMLINK = cp -f $(1) $(2)
    # Windows uses ; for path separator in some contexts, but MSYS uses :
    PATH_SEP := :
    # rm on MSYS2 handles both path styles
    RM := rm -f
    RMDIR := rm -rf
    MKDIR := mkdir -p
    # Windows timeout command has different syntax; use MSYS timeout or skip
    TIMEOUT_CMD = timeout
    # Null device
    NULL_DEV := /dev/null
else
    EXE_EXT :=
    CREATE_SYMLINK = ln -sf $(1) $(2)
    PATH_SEP := :
    RM := rm -f
    RMDIR := rm -rf
    MKDIR := mkdir -p
    TIMEOUT_CMD := timeout
    NULL_DEV := /dev/null
endif

#------------------------------------------------------------------------------
# Configuration
#------------------------------------------------------------------------------

# Default optimization level for GCC when compiling generated C
OPT_LEVEL ?= -O2

# ASAN options for test runs
export ASAN_OPTIONS ?= detect_leaks=0

# Directories
BIN_DIR := bin
LOG_DIR := log
SRC_DIR := src
TEST_DIR := tests/integration
ERROR_TEST_DIR := tests/integration/errors
EXPLORE_DIR := tests/exploratory
EXPLORE_ERROR_DIR := tests/exploratory/errors

# Sindarin compiler binary (can be overridden for backend-specific tests)
SN := $(BIN_DIR)/sn$(EXE_EXT)

# Colors (for terminal output)
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
BOLD := \033[1m
NC := \033[0m

#------------------------------------------------------------------------------
# Canned recipes for reducing duplication
#------------------------------------------------------------------------------

# Build a single backend
# Usage: $(call BUILD_BACKEND,DisplayName,backend-id)
define BUILD_BACKEND
	@$(MKDIR) $(BIN_DIR) $(LOG_DIR)
	@echo "Building compiler with $(1) backend..."
	@$(MAKE) -C $(SRC_DIR) clean PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT)
	@$(MAKE) -C $(SRC_DIR) BACKEND=$(2) PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) > $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) tests PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) >> $(LOG_DIR)/build-output.log 2>&1
	@cat $(LOG_DIR)/build-output.log
	@cp -n etc/*.cfg $(BIN_DIR)/ 2>>$(LOG_DIR)/build-warnings.log || true
	@find $(BIN_DIR) -maxdepth 1 \( -name "*.d" -o -name "*.o" \) -delete 2>$(NULL_DEV) || true
endef

# Run all tests for a backend
# Usage: $(call TEST_BACKEND,DisplayName,backend-suffix)
define TEST_BACKEND
	@echo ""
	@echo "$(BOLD)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(BOLD)  $(1) Backend$(NC)"
	@echo "$(BOLD)═══════════════════════════════════════════════════════════$(NC)"
	@$(MAKE) --no-print-directory test-unit
	@$(MAKE) --no-print-directory test-integration test-integration-errors SN=$(BIN_DIR)/sn-$(2)$(EXE_EXT)
	@$(MAKE) --no-print-directory test-explore test-explore-errors SN=$(BIN_DIR)/sn-$(2)$(EXE_EXT)
	@$(MAKE) --no-print-directory test-sdk SN=$(BIN_DIR)/sn-$(2)$(EXE_EXT)
endef

#------------------------------------------------------------------------------
# Default target
#------------------------------------------------------------------------------
all: build

#------------------------------------------------------------------------------
# build - Build compiler with all backends (GCC, Clang, TinyCC)
# Produces: sn-gcc, sn-clang, sn-tcc with 'sn' symlink/copy pointing to sn-gcc
#------------------------------------------------------------------------------
build:
	@$(MKDIR) $(BIN_DIR) $(LOG_DIR)
	@echo "Building compiler with all backends (platform: $(PLATFORM))..."
	@$(MAKE) -C $(SRC_DIR) clean PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT)
	@# Build GCC backend first (includes test binary)
	@echo "  Building GCC backend..."
	@$(MAKE) -C $(SRC_DIR) BACKEND=gcc PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) > $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) tests PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) >> $(LOG_DIR)/build-output.log 2>&1
	@# Build Clang backend (compiler binary + runtime)
	@echo "  Building Clang backend..."
	@$(MAKE) -C $(SRC_DIR) runtime-clang PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) >> $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) BACKEND=clang PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) ../$(BIN_DIR)/sn-clang$(EXE_EXT) >> $(LOG_DIR)/build-output.log 2>&1
	@# Build TinyCC backend (compiler binary + runtime)
	@echo "  Building TinyCC backend..."
	@$(MAKE) -C $(SRC_DIR) runtime-tinycc PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) >> $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) BACKEND=tinycc PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) ../$(BIN_DIR)/sn-tcc$(EXE_EXT) >> $(LOG_DIR)/build-output.log 2>&1
	@# Set sn symlink/copy to gcc by default
	@$(RM) $(BIN_DIR)/sn$(EXE_EXT)
	$(call CREATE_SYMLINK,sn-gcc$(EXE_EXT),$(BIN_DIR)/sn$(EXE_EXT))
	@cp -n etc/*.cfg $(BIN_DIR)/ 2>>$(LOG_DIR)/build-warnings.log || true
	@echo "Built: sn-gcc$(EXE_EXT), sn-clang$(EXE_EXT), sn-tcc$(EXE_EXT) (sn -> sn-gcc)"
	@find $(BIN_DIR) -maxdepth 1 \( -name "*.d" -o -name "*.o" \) -delete 2>$(NULL_DEV) || true

#------------------------------------------------------------------------------
# rebuild - Clean and build (for when you need a fresh build)
#------------------------------------------------------------------------------
rebuild: clean build

#------------------------------------------------------------------------------
# Backend-specific builds
#------------------------------------------------------------------------------
build-gcc:
	$(call BUILD_BACKEND,GCC,gcc)

build-clang:
	$(call BUILD_BACKEND,Clang,clang)
	@# On Unix, clang backend uses lib/gcc runtime (gcc/clang produce compatible objects)
	@$(MAKE) -C $(SRC_DIR) runtime BACKEND=gcc PLATFORM=$(PLATFORM) EXE_EXT=$(EXE_EXT) > /dev/null 2>&1

build-tcc:
	$(call BUILD_BACKEND,TinyCC,tinycc)

#------------------------------------------------------------------------------
# clean - Remove build artifacts (preserves .cfg config files)
#------------------------------------------------------------------------------
clean:
	@$(RM) $(BIN_DIR)/sn$(EXE_EXT) $(BIN_DIR)/sn-*$(EXE_EXT) $(BIN_DIR)/tests$(EXE_EXT)
	@$(RM) $(BIN_DIR)/*.o $(BIN_DIR)/*.d
	@$(RM) $(BIN_DIR)/hello-world$(EXE_EXT)
	@$(RMDIR) $(LOG_DIR)/*
	@$(MKDIR) $(BIN_DIR) $(LOG_DIR)

#------------------------------------------------------------------------------
# run - Compile and run samples/main.sn
#------------------------------------------------------------------------------
run:
	@$(MKDIR) $(LOG_DIR)
	$(SN) samples/main.sn -o $(BIN_DIR)/hello-world$(EXE_EXT) -l 3 -g $(OPT_LEVEL) > $(LOG_DIR)/run-output.log 2>&1
	@$(TIMEOUT_CMD) 5s $(BIN_DIR)/hello-world$(EXE_EXT) || $(BIN_DIR)/hello-world$(EXE_EXT)

#------------------------------------------------------------------------------
# test - Run all tests with all backends
#------------------------------------------------------------------------------
test: test-gcc test-clang test-tcc

#------------------------------------------------------------------------------
# Backend-specific test suites
#------------------------------------------------------------------------------
test-gcc:
	$(call TEST_BACKEND,GCC,gcc)

test-clang:
	$(call TEST_BACKEND,Clang,clang)

test-tcc:
	@# TinyCC lacks __thread support - exclude thread+interceptor tests that require TLS
	@SN_EXCLUDE_TESTS="test_interceptor_thread_counter" $(MAKE) --no-print-directory _test-tcc-impl

_test-tcc-impl:
	$(call TEST_BACKEND,TinyCC,tcc)

#------------------------------------------------------------------------------
# test-unit - Run unit tests
# Note: Unit tests use a single GCC-compiled binary since they test compiler
# internals. Backend-specific code generation is tested via integration tests.
#------------------------------------------------------------------------------
test-unit:
	@echo ""
	@echo "$(BOLD)Unit Tests$(NC)"
	@echo "============================================================"
	@$(MKDIR) $(LOG_DIR)
	@if $(BIN_DIR)/tests$(EXE_EXT) > $(LOG_DIR)/test-output.log 2>&1; then \
		cat $(LOG_DIR)/test-output.log; \
		echo ""; \
		echo "------------------------------------------------------------"; \
		printf "$(GREEN)Unit tests passed$(NC)\n"; \
	else \
		cat $(LOG_DIR)/test-output.log; \
		echo ""; \
		echo "------------------------------------------------------------"; \
		printf "$(RED)Unit tests failed$(NC)\n"; \
		exit 1; \
	fi

#------------------------------------------------------------------------------
# Integration and exploratory tests (delegated to unified test runner)
#------------------------------------------------------------------------------
test-integration:
	@./scripts/run_tests.sh integration $(SN)

test-integration-errors:
	@./scripts/run_tests.sh integration-errors $(SN)

test-explore:
	@./scripts/run_tests.sh explore $(SN)

test-explore-errors:
	@./scripts/run_tests.sh explore-errors $(SN)

test-sdk:
	@./scripts/run_tests.sh sdk $(SN)

#------------------------------------------------------------------------------
# test-threading - Run all threading tests (positive and negative)
#------------------------------------------------------------------------------
test-threading:
	@./scripts/test_threading.sh

#------------------------------------------------------------------------------
# assembly - Assemble and link assembly files
#------------------------------------------------------------------------------
assembly:
	@$(MKDIR) $(BIN_DIR)
ifeq ($(PLATFORM),windows)
	nasm -f win64 $(BIN_DIR)/hello-world.asm -o $(BIN_DIR)/hello-world.o
	gcc $(BIN_DIR)/hello-world.o -o $(BIN_DIR)/hello-world$(EXE_EXT)
else
	nasm -f elf64 $(BIN_DIR)/hello-world.asm -o $(BIN_DIR)/hello-world.o
	gcc -no-pie $(BIN_DIR)/hello-world.o -o $(BIN_DIR)/hello-world
endif
	@$(TIMEOUT_CMD) 5s ./$(BIN_DIR)/hello-world$(EXE_EXT) || ./$(BIN_DIR)/hello-world$(EXE_EXT)

#------------------------------------------------------------------------------
# measure-optimization - Measure optimization impact
#------------------------------------------------------------------------------
ifeq ($(PLATFORM),windows)
MEASURE_TEMP := $(TEMP)/sn_optimization_test
else
MEASURE_TEMP := /tmp/sn_optimization_test
endif
MEASURE_TESTS := factorial.sn loops.sn string_interp.sn const_fold.sn edge_cases.sn native_ops.sn tail_recursion.sn

measure-optimization:
	@$(MKDIR) $(MEASURE_TEMP)
	@echo "=============================================="
	@echo "Sn Compiler Optimization Impact Analysis"
	@echo "=============================================="
	@echo ""
	@printf "%-20s %10s %10s %10s %10s %10s\n" "Test" "O0 Lines" "O1 Lines" "O2 Lines" "O0->O2 %" "O1->O2 %"
	@echo "--------------------------------------------------------------------------------"
	@total_o0=0; total_o1=0; total_o2=0; \
	for test_file in $(MEASURE_TESTS); do \
		test_path="$(TEST_DIR)/$$test_file"; \
		test_name="$${test_file%.sn}"; \
		[ -f "$$test_path" ] || continue; \
		$(SN) "$$test_path" -o "$(MEASURE_TEMP)/$${test_name}_o0.c" -O0 2>/dev/null; \
		$(SN) "$$test_path" -o "$(MEASURE_TEMP)/$${test_name}_o1.c" -O1 2>/dev/null; \
		$(SN) "$$test_path" -o "$(MEASURE_TEMP)/$${test_name}_o2.c" -O2 2>/dev/null; \
		lines_o0=$$(wc -l < "$(MEASURE_TEMP)/$${test_name}_o0.c"); \
		lines_o1=$$(wc -l < "$(MEASURE_TEMP)/$${test_name}_o1.c"); \
		lines_o2=$$(wc -l < "$(MEASURE_TEMP)/$${test_name}_o2.c"); \
		reduction_o0_o2=$$(awk "BEGIN {printf \"%.1f\", (($$lines_o0 - $$lines_o2) * 100) / $$lines_o0}"); \
		reduction_o1_o2=$$(awk "BEGIN {printf \"%.1f\", (($$lines_o1 - $$lines_o2) * 100) / $$lines_o1}"); \
		printf "%-20s %10d %10d %10d %9s%% %9s%%\n" "$$test_name" "$$lines_o0" "$$lines_o1" "$$lines_o2" "$$reduction_o0_o2" "$$reduction_o1_o2"; \
		total_o0=$$((total_o0 + lines_o0)); \
		total_o1=$$((total_o1 + lines_o1)); \
		total_o2=$$((total_o2 + lines_o2)); \
	done; \
	echo "--------------------------------------------------------------------------------"; \
	total_reduction_o0=$$(awk "BEGIN {printf \"%.1f\", (($$total_o0 - $$total_o2) * 100) / $$total_o0}"); \
	total_reduction_o1=$$(awk "BEGIN {printf \"%.1f\", (($$total_o1 - $$total_o2) * 100) / $$total_o1}"); \
	printf "%-20s %10d %10d %10d %9s%% %9s%%\n" "TOTAL" "$$total_o0" "$$total_o1" "$$total_o2" "$$total_reduction_o0" "$$total_reduction_o1"; \
	rm -rf $(MEASURE_TEMP)

#------------------------------------------------------------------------------
# benchmark - Run multi-language benchmark suite
#------------------------------------------------------------------------------
BENCHMARK_DIR := benchmark

benchmark:
	@echo "Running Sindarin benchmark suite..."
	@cd $(BENCHMARK_DIR) && ./run_all.sh

#------------------------------------------------------------------------------
# help - Show available targets
#------------------------------------------------------------------------------
help:
	@echo "Sn Compiler - Available targets:"
	@echo ""
	@echo "  Build:"
	@echo "    make              Build compiler (all backends, incremental)"
	@echo "    make build        Build compiler with all backends (sn-gcc, sn-clang, sn-tcc)"
	@echo "    make rebuild      Clean and build (full rebuild)"
	@echo "    make build-gcc    Build compiler with GCC backend only"
	@echo "    make build-clang  Build compiler with Clang backend only"
	@echo "    make build-tcc    Build compiler with TinyCC backend only"
	@echo "    make clean        Remove build artifacts (preserves .cfg files)"
	@echo ""
	@echo "  Run:"
	@echo "    make run          Compile and run samples/main.sn"
	@echo ""
	@echo "  Test:"
	@echo "    make test         Run ALL tests with ALL backends (GCC, Clang, TinyCC)"
	@echo "    make test-gcc     Run all tests with GCC backend"
	@echo "    make test-clang   Run all tests with Clang backend"
	@echo "    make test-tcc     Run all tests with TinyCC backend"
	@echo "    make test-unit    Run unit tests only"
	@echo "    make test-sdk     Run SDK tests only"
	@echo ""
	@echo "  Analysis:"
	@echo "    make assembly             Assemble and link assembly files"
	@echo "    make measure-optimization Measure optimization impact"
	@echo "    make benchmark            Run multi-language benchmark suite"
	@echo ""
	@echo "Options:"
	@echo "  OPT_LEVEL=-O0     Set GCC optimization level (default: -O2)"
	@echo "  ASAN_OPTIONS=...  Override ASAN options (default: detect_leaks=0)"
	@echo ""
	@echo "Backend selection:"
	@echo "  bin/sn-gcc        Use GCC backend directly"
	@echo "  bin/sn-clang      Use Clang backend directly"
	@echo "  bin/sn-tcc        Use TinyCC backend directly"
	@echo "  bin/sn            Symlink to sn-gcc (default)"
	@echo ""
	@echo "  Or set SN_CC environment variable:"
	@echo "    SN_CC=gcc bin/sn ...    Use GCC backend"
	@echo "    SN_CC=clang bin/sn ...  Use Clang backend"
	@echo "    SN_CC=tcc bin/sn ...    Use TinyCC backend"
	@echo ""
	@echo "Config files (in bin/, copied from etc/ during build):"
	@echo "  sn-gcc.cfg        Config for sn-gcc"
	@echo "  sn-clang.cfg      Config for sn-clang"
	@echo "  sn-tcc.cfg        Config for sn-tcc"
