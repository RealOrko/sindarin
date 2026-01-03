# Sn Compiler - Root Makefile
# Consolidates all build and test operations

.PHONY: all build clean run test test-unit test-integration test-integration-errors test-explore test-explore-errors test-threading assembly measure-optimization benchmark help

# Default optimization level for GCC when compiling generated C
OPT_LEVEL ?= -O2

# Directories
BIN_DIR := bin
LOG_DIR := log
SRC_DIR := src
TEST_DIR := tests/integration
ERROR_TEST_DIR := tests/integration/errors
TEMP_DIR := /tmp/sn_integration_tests
EXPLORE_DIR := tests/exploratory
EXPLORE_ERROR_DIR := tests/exploratory/errors
EXPLORE_OUT := $(EXPLORE_DIR)/output

# Colors (for terminal output)
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
BOLD := \033[1m
NC := \033[0m

#------------------------------------------------------------------------------
# Default target
#------------------------------------------------------------------------------
all: build

#------------------------------------------------------------------------------
# build - Build compiler and test binary (no tests run)
#------------------------------------------------------------------------------
build: clean
	@mkdir -p $(BIN_DIR) $(LOG_DIR)
	@echo "Building compiler..."
	@$(MAKE) -C $(SRC_DIR) clean
	@$(MAKE) -C $(SRC_DIR) > $(LOG_DIR)/build-output.log 2>&1
	@$(MAKE) -C $(SRC_DIR) tests >> $(LOG_DIR)/build-output.log 2>&1
	@cat $(LOG_DIR)/build-output.log
	@# Clean up intermediate files, keeping only required runtime objects
	@find $(BIN_DIR) -name "*.d" -delete
	@find $(BIN_DIR) -name "*.o" ! -name "arena.o" ! -name "debug.o" ! -name "runtime.o" ! -name "runtime_arena.o" ! -name "runtime_string.o" ! -name "runtime_array.o" ! -name "runtime_text_file.o" ! -name "runtime_binary_file.o" ! -name "runtime_io.o" ! -name "runtime_byte.o" ! -name "runtime_path.o" ! -name "runtime_date.o" ! -name "runtime_time.o" ! -name "runtime_thread.o" -delete

#------------------------------------------------------------------------------
# clean - Remove build artifacts
#------------------------------------------------------------------------------
clean:
	@rm -rf $(BIN_DIR)/*
	@rm -rf $(LOG_DIR)/*
	@mkdir -p $(BIN_DIR) $(LOG_DIR)

#------------------------------------------------------------------------------
# run - Compile and run samples/main.sn
#------------------------------------------------------------------------------
run:
	@mkdir -p $(LOG_DIR)
	$(BIN_DIR)/sn samples/main.sn -o $(BIN_DIR)/hello-world -l 3 -g $(OPT_LEVEL) > $(LOG_DIR)/run-output.log 2>&1
	@$(BIN_DIR)/hello-world > $(LOG_DIR)/hello-world-output.log 2>&1
	@cat $(LOG_DIR)/hello-world-output.log

#------------------------------------------------------------------------------
# test - Run all tests (unit, integration, exploratory)
#------------------------------------------------------------------------------
test: test-unit test-integration test-integration-errors test-explore test-explore-errors

#------------------------------------------------------------------------------
# test-unit - Run unit tests
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
# test-integration - Run integration tests (positive tests that should pass)
#------------------------------------------------------------------------------
test-integration:
	@echo ""
	@echo "$(BOLD)Integration Tests$(NC)"
	@echo "============================================================"
	@mkdir -p $(TEMP_DIR)
	@passed=0; failed=0; skipped=0; \
	for test_file in $(TEST_DIR)/*.sn; do \
		[ -f "$$test_file" ] || continue; \
		test_name=$$(basename "$$test_file" .sn); \
		expected_file="$${test_file%.sn}.expected"; \
		panic_file="$${test_file%.sn}.panic"; \
		exe_file="$(TEMP_DIR)/$$test_name"; \
		output_file="$(TEMP_DIR)/$$test_name.out"; \
		printf "  %-45s " "$$test_name"; \
		if [ ! -f "$$expected_file" ]; then \
			printf "$(YELLOW)SKIP$(NC) (no .expected)\n"; \
			skipped=$$((skipped + 1)); \
			continue; \
		fi; \
		if ! $(BIN_DIR)/sn "$$test_file" -o "$$exe_file" -l 1 -g -O0 2>"$(TEMP_DIR)/$$test_name.compile_err"; then \
			printf "$(RED)FAIL$(NC) (compile error)\n"; \
			head -3 "$(TEMP_DIR)/$$test_name.compile_err" | sed 's/^/    /'; \
			failed=$$((failed + 1)); \
			continue; \
		fi; \
		ASAN_OPTIONS=detect_leaks=0 timeout 5s "$$exe_file" > "$$output_file" 2>&1; \
		run_exit_code=$$?; \
		if [ -f "$$panic_file" ]; then \
			if [ $$run_exit_code -eq 0 ]; then \
				printf "$(RED)FAIL$(NC) (expected panic)\n"; \
				failed=$$((failed + 1)); \
				continue; \
			fi; \
		else \
			if [ $$run_exit_code -ne 0 ]; then \
				printf "$(RED)FAIL$(NC) (runtime error)\n"; \
				head -3 "$$output_file" | sed 's/^/    /'; \
				failed=$$((failed + 1)); \
				continue; \
			fi; \
		fi; \
		if diff -q "$$output_file" "$$expected_file" > /dev/null 2>&1; then \
			printf "$(GREEN)PASS$(NC)\n"; \
			passed=$$((passed + 1)); \
		else \
			printf "$(RED)FAIL$(NC) (output mismatch)\n"; \
			echo "    Expected: $$(head -1 "$$expected_file")"; \
			echo "    Got:      $$(head -1 "$$output_file")"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "------------------------------------------------------------"; \
	printf "Results: $(GREEN)%d passed$(NC), $(RED)%d failed$(NC), $(YELLOW)%d skipped$(NC)\n" $$passed $$failed $$skipped; \
	rm -rf $(TEMP_DIR); \
	if [ $$failed -gt 0 ]; then exit 1; fi

#------------------------------------------------------------------------------
# test-integration-errors - Run compile error tests (tests that should fail)
#------------------------------------------------------------------------------
test-integration-errors:
	@echo ""
	@echo "$(BOLD)Integration Error Tests$(NC) (expected compile failures)"
	@echo "============================================================"
	@mkdir -p $(TEMP_DIR)
	@passed=0; failed=0; skipped=0; \
	for test_file in $(ERROR_TEST_DIR)/*.sn; do \
		[ -f "$$test_file" ] || continue; \
		test_name=$$(basename "$$test_file" .sn); \
		expected_file="$${test_file%.sn}.expected"; \
		compile_err="$(TEMP_DIR)/$$test_name.compile_err"; \
		printf "  %-45s " "$$test_name"; \
		if [ ! -f "$$expected_file" ]; then \
			printf "$(YELLOW)SKIP$(NC) (no .expected)\n"; \
			skipped=$$((skipped + 1)); \
			continue; \
		fi; \
		if $(BIN_DIR)/sn "$$test_file" -o "$(TEMP_DIR)/$$test_name" -l 1 2>"$$compile_err"; then \
			printf "$(RED)FAIL$(NC) (should not compile)\n"; \
			failed=$$((failed + 1)); \
			continue; \
		fi; \
		if grep -qF "$$(cat "$$expected_file" | head -1)" "$$compile_err"; then \
			printf "$(GREEN)PASS$(NC)\n"; \
			passed=$$((passed + 1)); \
		else \
			printf "$(RED)FAIL$(NC) (wrong error)\n"; \
			echo "    Expected: $$(head -1 "$$expected_file")"; \
			echo "    Got:      $$(head -1 "$$compile_err" | sed 's/.*error[0m: //')"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "------------------------------------------------------------"; \
	printf "Results: $(GREEN)%d passed$(NC), $(RED)%d failed$(NC), $(YELLOW)%d skipped$(NC)\n" $$passed $$failed $$skipped; \
	rm -rf $(TEMP_DIR); \
	if [ $$failed -gt 0 ]; then exit 1; fi

#------------------------------------------------------------------------------
# test-explore - Run exploratory tests (positive tests that should pass)
#------------------------------------------------------------------------------
test-explore:
	@echo ""
	@echo "$(BOLD)Exploratory Tests$(NC)"
	@echo "============================================================"
	@rm -rf $(EXPLORE_OUT) && mkdir -p $(EXPLORE_OUT)
	@passed=0; failed=0; skipped=0; \
	for test_file in $(EXPLORE_DIR)/test_*.sn; do \
		[ -f "$$test_file" ] || continue; \
		test_name=$$(basename "$$test_file" .sn); \
		expected_file="$${test_file%.sn}.expected"; \
		exe_file="$(EXPLORE_OUT)/$$test_name"; \
		output_file="$(EXPLORE_OUT)/$$test_name.out"; \
		printf "  %-45s " "$$test_name"; \
		if ! timeout 10s $(BIN_DIR)/sn "$$test_file" -o "$$exe_file" -g -O0 2>"$(EXPLORE_OUT)/$$test_name.compile_err"; then \
			printf "$(RED)FAIL$(NC) (compile error)\n"; \
			head -3 "$(EXPLORE_OUT)/$$test_name.compile_err" | sed 's/^/    /'; \
			failed=$$((failed + 1)); \
			continue; \
		fi; \
		ASAN_OPTIONS=detect_leaks=0 timeout 30s "$$exe_file" > "$$output_file" 2>&1; \
		run_exit_code=$$?; \
		if [ $$run_exit_code -ne 0 ]; then \
			printf "$(RED)FAIL$(NC) (exit code: $$run_exit_code)\n"; \
			head -3 "$$output_file" | sed 's/^/    /'; \
			failed=$$((failed + 1)); \
			continue; \
		fi; \
		if [ -f "$$expected_file" ]; then \
			if diff -q "$$output_file" "$$expected_file" > /dev/null 2>&1; then \
				printf "$(GREEN)PASS$(NC)\n"; \
				passed=$$((passed + 1)); \
			else \
				printf "$(RED)FAIL$(NC) (output mismatch)\n"; \
				echo "    Expected: $$(head -1 "$$expected_file")"; \
				echo "    Got:      $$(head -1 "$$output_file")"; \
				failed=$$((failed + 1)); \
			fi; \
		else \
			printf "$(GREEN)PASS$(NC)\n"; \
			passed=$$((passed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "------------------------------------------------------------"; \
	printf "Results: $(GREEN)%d passed$(NC), $(RED)%d failed$(NC), $(YELLOW)%d skipped$(NC)\n" $$passed $$failed $$skipped; \
	if [ $$failed -gt 0 ]; then exit 1; fi

#------------------------------------------------------------------------------
# test-explore-errors - Run exploratory error tests (expected compile failures)
#------------------------------------------------------------------------------
test-explore-errors:
	@echo ""
	@echo "$(BOLD)Exploratory Error Tests$(NC) (expected compile failures)"
	@echo "============================================================"
	@mkdir -p $(EXPLORE_OUT)
	@passed=0; failed=0; skipped=0; \
	for test_file in $(EXPLORE_ERROR_DIR)/*.sn; do \
		[ -f "$$test_file" ] || continue; \
		test_name=$$(basename "$$test_file" .sn); \
		expected_file="$${test_file%.sn}.expected"; \
		compile_err="$(EXPLORE_OUT)/$$test_name.compile_err"; \
		printf "  %-45s " "$$test_name"; \
		if [ ! -f "$$expected_file" ]; then \
			printf "$(YELLOW)SKIP$(NC) (no .expected)\n"; \
			skipped=$$((skipped + 1)); \
			continue; \
		fi; \
		if $(BIN_DIR)/sn "$$test_file" -o "$(EXPLORE_OUT)/$$test_name" -l 1 2>"$$compile_err"; then \
			printf "$(RED)FAIL$(NC) (should not compile)\n"; \
			failed=$$((failed + 1)); \
			continue; \
		fi; \
		if grep -qF "$$(cat "$$expected_file" | head -1)" "$$compile_err"; then \
			printf "$(GREEN)PASS$(NC)\n"; \
			passed=$$((passed + 1)); \
		else \
			printf "$(RED)FAIL$(NC) (wrong error)\n"; \
			echo "    Expected: $$(head -1 "$$expected_file")"; \
			echo "    Got:      $$(head -1 "$$compile_err" | sed 's/.*error[0m: //')"; \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	echo ""; \
	echo "------------------------------------------------------------"; \
	printf "Results: $(GREEN)%d passed$(NC), $(RED)%d failed$(NC), $(YELLOW)%d skipped$(NC)\n" $$passed $$failed $$skipped; \
	if [ $$failed -gt 0 ]; then exit 1; fi

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
	@./$(BIN_DIR)/hello-world

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
		$(BIN_DIR)/sn "$$test_path" -o "$(MEASURE_TEMP)/$${test_name}_o0.c" -O0 2>/dev/null; \
		$(BIN_DIR)/sn "$$test_path" -o "$(MEASURE_TEMP)/$${test_name}_o1.c" -O1 2>/dev/null; \
		$(BIN_DIR)/sn "$$test_path" -o "$(MEASURE_TEMP)/$${test_name}_o2.c" -O2 2>/dev/null; \
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
	@echo "  make              Build compiler (default)"
	@echo "  make build        Build compiler and test binary"
	@echo "  make clean        Remove build artifacts"
	@echo "  make run          Compile and run samples/main.sn"
	@echo ""
	@echo "  make test         Run all tests"
	@echo "  make test-unit    Run unit tests"
	@echo "  make test-integration        Run integration tests (positive)"
	@echo "  make test-integration-errors Run integration error tests (negative)"
	@echo "  make test-explore            Run exploratory tests (positive)"
	@echo "  make test-explore-errors     Run exploratory error tests (negative)"
	@echo ""
	@echo "  make assembly     Assemble and link assembly files"
	@echo "  make measure-optimization  Measure optimization impact"
	@echo "  make benchmark    Run multi-language benchmark suite"
	@echo ""
	@echo "Options:"
	@echo "  OPT_LEVEL=-O0     Set GCC optimization level (default: -O2)"
