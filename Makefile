# Sn Compiler - Root Makefile
# Consolidates all build and test operations

#------------------------------------------------------------------------------
# Phony targets (grouped by category)
#------------------------------------------------------------------------------
.PHONY: all build rebuild run clean
.PHONY: build-gcc build-clang build-tcc
.PHONY: test test-unit test-integration test-integration-errors
.PHONY: test-explore test-explore-errors test-threading
.PHONY: test-gcc test-clang test-tcc
.PHONY: assembly measure-optimization benchmark help

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
SN := $(BIN_DIR)/sn

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
	@mkdir -p $(BIN_DIR) $(LOG_DIR)
	@echo "Building compiler with $(1) backend..."
	@$(MAKE) -C $(SRC_DIR) clean
	@$(MAKE) -C $(SRC_DIR) BACKEND=$(2) > $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) tests >> $(LOG_DIR)/build-output.log 2>&1
	@cat $(LOG_DIR)/build-output.log
	@cp -n etc/*.cfg $(BIN_DIR)/ 2>>$(LOG_DIR)/build-warnings.log || true
	@find $(BIN_DIR) -maxdepth 1 \( -name "*.d" -o -name "*.o" \) -delete
endef

# Run all tests for a backend
# Usage: $(call TEST_BACKEND,DisplayName,backend-suffix)
define TEST_BACKEND
	@echo ""
	@echo "$(BOLD)═══════════════════════════════════════════════════════════$(NC)"
	@echo "$(BOLD)  $(1) Backend$(NC)"
	@echo "$(BOLD)═══════════════════════════════════════════════════════════$(NC)"
	@$(MAKE) --no-print-directory test-unit
	@$(MAKE) --no-print-directory test-integration test-integration-errors SN=$(BIN_DIR)/sn-$(2)
	@$(MAKE) --no-print-directory test-explore test-explore-errors SN=$(BIN_DIR)/sn-$(2)
endef

#------------------------------------------------------------------------------
# Default target
#------------------------------------------------------------------------------
all: build

#------------------------------------------------------------------------------
# build - Build compiler with all backends (GCC, Clang, TinyCC)
# Produces: sn-gcc, sn-clang, sn-tcc with 'sn' symlink pointing to sn-gcc
#------------------------------------------------------------------------------
build:
	@mkdir -p $(BIN_DIR) $(LOG_DIR)
	@echo "Building compiler with all backends..."
	@$(MAKE) -C $(SRC_DIR) clean
	@# Build GCC backend first (includes test binary)
	@echo "  Building GCC backend..."
	@$(MAKE) -C $(SRC_DIR) BACKEND=gcc > $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) tests >> $(LOG_DIR)/build-output.log 2>&1
	@# Build Clang backend (compiler binary + runtime)
	@echo "  Building Clang backend..."
	@$(MAKE) -C $(SRC_DIR) runtime-clang >> $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) BACKEND=clang ../$(BIN_DIR)/sn-clang >> $(LOG_DIR)/build-output.log 2>&1
	@# Build TinyCC backend (compiler binary + runtime)
	@echo "  Building TinyCC backend..."
	@$(MAKE) -C $(SRC_DIR) runtime-tinycc >> $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) BACKEND=tinycc ../$(BIN_DIR)/sn-tcc >> $(LOG_DIR)/build-output.log 2>&1
	@# Set sn symlink to gcc by default
	@rm -f $(BIN_DIR)/sn
	@ln -s sn-gcc $(BIN_DIR)/sn
	@cp -n etc/*.cfg $(BIN_DIR)/ 2>>$(LOG_DIR)/build-warnings.log || true
	@echo "Built: sn-gcc, sn-clang, sn-tcc (sn -> sn-gcc)"
	@find $(BIN_DIR) -maxdepth 1 \( -name "*.d" -o -name "*.o" \) -delete

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

build-tcc:
	$(call BUILD_BACKEND,TinyCC,tinycc)

#------------------------------------------------------------------------------
# clean - Remove build artifacts (preserves .cfg config files)
#------------------------------------------------------------------------------
clean:
	@rm -f $(BIN_DIR)/sn $(BIN_DIR)/sn-* $(BIN_DIR)/tests
	@rm -f $(BIN_DIR)/*.o $(BIN_DIR)/*.d
	@rm -f $(BIN_DIR)/hello-world
	@rm -rf $(LOG_DIR)/*
	@mkdir -p $(BIN_DIR) $(LOG_DIR)

#------------------------------------------------------------------------------
# run - Compile and run samples/main.sn
#------------------------------------------------------------------------------
run:
	@mkdir -p $(LOG_DIR)
	$(SN) samples/main.sn -o $(BIN_DIR)/hello-world -l 3 -g $(OPT_LEVEL) > $(LOG_DIR)/run-output.log 2>&1
	@timeout 5s $(BIN_DIR)/hello-world

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
	@mkdir -p $(LOG_DIR)
	@if $(BIN_DIR)/tests > $(LOG_DIR)/test-output.log 2>&1; then \
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

#------------------------------------------------------------------------------
# test-threading - Run all threading tests (positive and negative)
#------------------------------------------------------------------------------
test-threading:
	@./scripts/test_threading.sh

#------------------------------------------------------------------------------
# assembly - Assemble and link assembly files
#------------------------------------------------------------------------------
assembly:
	@mkdir -p $(BIN_DIR)
	nasm -f elf64 $(BIN_DIR)/hello-world.asm -o $(BIN_DIR)/hello-world.o
	gcc -no-pie $(BIN_DIR)/hello-world.o -o $(BIN_DIR)/hello-world
	@timeout 5s ./$(BIN_DIR)/hello-world

#------------------------------------------------------------------------------
# measure-optimization - Measure optimization impact
#------------------------------------------------------------------------------
MEASURE_TEMP := /tmp/sn_optimization_test
MEASURE_TESTS := factorial.sn loops.sn string_interp.sn const_fold.sn edge_cases.sn native_ops.sn tail_recursion.sn

measure-optimization:
	@mkdir -p $(MEASURE_TEMP)
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
	@echo "  sn.gcc.cfg        Config for sn-gcc"
	@echo "  sn.clang.cfg      Config for sn-clang"
	@echo "  sn.tcc.cfg        Config for sn-tcc"
