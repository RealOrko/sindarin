# Backend selection (default: gcc)
# Use BACKEND=gcc|clang|tinycc to select which backend to build runtime for
BACKEND ?= gcc

#------------------------------------------------------------------------------
# Platform Detection (can be passed from parent Makefile)
#------------------------------------------------------------------------------
PLATFORM ?= $(if $(filter Windows_NT,$(OS)),windows,$(if $(filter MINGW% MSYS% CYGWIN%,$(shell uname -s 2>/dev/null)),windows,linux))

# Platform-specific settings
ifeq ($(PLATFORM),windows)
    EXE_EXT ?= .exe
    # Windows doesn't have -lpthread (use -pthread instead or nothing for MinGW)
    PTHREAD_FLAG := -pthread
    # Windows uses ws2_32 for sockets
    SOCKET_LIBS := -lws2_32
else
    EXE_EXT ?=
    PTHREAD_FLAG := -lpthread
    SOCKET_LIBS :=
endif

# Backend-specific compiler and flags
# Note: Don't use -flto for runtime objects - they need to link in both release and debug modes
ifeq ($(BACKEND),gcc)
BACKEND_CC = gcc
BACKEND_CFLAGS =
BACKEND_LDFLAGS =
else ifeq ($(BACKEND),clang)
BACKEND_CC = clang
BACKEND_CFLAGS =
BACKEND_LDFLAGS =
else ifeq ($(BACKEND),tinycc)
BACKEND_CC = tcc
# TinyCC doesn't support -flto or -fsanitize
BACKEND_CFLAGS =
BACKEND_LDFLAGS =
endif

# Compiler for building sn itself (always gcc)
CC = gcc

# Release build (default): optimized, no sanitizer
CFLAGS = -Wall -Wextra -std=c99 -O3 -flto -MMD -MP -D_GNU_SOURCE -I. -Iruntime
LDFLAGS = -flto -O3 $(PTHREAD_FLAG) $(SOCKET_LIBS)

# Debug build: use 'make DEBUG=1' for sanitizer and debug symbols
ifdef DEBUG
CFLAGS = -Wall -Wextra -std=c99 -g -fsanitize=address -fno-omit-frame-pointer -MMD -MP -D_GNU_SOURCE -I. -Iruntime
LDFLAGS = -fsanitize=address $(PTHREAD_FLAG) -lm $(SOCKET_LIBS)
endif

SRCDIR = .
SRCS = arena.c file.c runtime.c token.c \
	lexer.c lexer/lexer_util.c lexer/lexer_scan.c \
	ast.c ast/ast_type.c ast/ast_expr.c ast/ast_stmt.c ast/ast_print.c \
	parser.c parser/parser_util.c parser/parser_expr.c parser/parser_stmt.c parser/parser_stmt_decl.c parser/parser_stmt_control.c \
	symbol_table.c symbol_table/symbol_table_core.c symbol_table/symbol_table_namespace.c symbol_table/symbol_table_thread.c \
	code_gen.c code_gen/code_gen_util.c code_gen/code_gen_expr.c code_gen/code_gen_expr_core.c code_gen/code_gen_expr_binary.c code_gen/code_gen_expr_lambda.c \
	code_gen/code_gen_expr_call.c code_gen/code_gen_expr_call_array.c code_gen/code_gen_expr_call_file.c \
	code_gen/code_gen_expr_call_string.c code_gen/code_gen_expr_call_time.c \
	code_gen/code_gen_expr_call_random.c code_gen/code_gen_expr_call_uuid.c \
	code_gen/code_gen_expr_array.c code_gen/code_gen_expr_static.c code_gen/code_gen_expr_string.c \
	code_gen/code_gen_expr_thread.c \
	code_gen/code_gen_stmt_core.c code_gen/code_gen_stmt_loop.c code_gen/code_gen_stmt_capture.c \
	optimizer.c optimizer/optimizer_util.c optimizer/optimizer_tail_call.c optimizer/optimizer_string.c \
	compiler.c debug.c diagnostic.c \
	type_checker.c type_checker/type_checker_util.c type_checker/type_checker_expr.c \
	type_checker/type_checker_expr_call.c type_checker/type_checker_expr_call_core.c \
	type_checker/type_checker_expr_call_array.c type_checker/type_checker_expr_call_string.c \
	type_checker/type_checker_expr_call_file.c type_checker/type_checker_expr_call_time.c \
	type_checker/type_checker_expr_call_net.c type_checker/type_checker_expr_call_random.c \
	type_checker/type_checker_expr_array.c \
	type_checker/type_checker_expr_lambda.c type_checker/type_checker_stmt.c \
	gcc_backend.c main.c \
	runtime/runtime_arena.c runtime/runtime_string.c runtime/runtime_array.c \
	runtime/runtime_text_file.c runtime/runtime_binary_file.c runtime/runtime_io.c \
	runtime/runtime_byte.c runtime/runtime_path.c runtime/runtime_date.c runtime/runtime_time.c \
	runtime/runtime_thread.c runtime/runtime_process.c runtime/runtime_net.c \
	runtime/runtime_random_core.c runtime/runtime_random_basic.c runtime/runtime_random_static.c runtime/runtime_random_choice.c runtime/runtime_random_collection.c runtime/runtime_random.c \
	runtime/runtime_uuid.c runtime/runtime_sha1.c runtime/runtime_env.c
HEADERS = arena.h file.h runtime.h token.h \
	lexer.h lexer/lexer_util.h lexer/lexer_scan.h \
	ast.h ast/ast_type.h ast/ast_expr.h ast/ast_stmt.h ast/ast_print.h \
	parser.h parser/parser_util.h parser/parser_expr.h parser/parser_stmt.h parser/parser_stmt_decl.h parser/parser_stmt_control.h \
	symbol_table.h symbol_table/symbol_table_core.h symbol_table/symbol_table_namespace.h symbol_table/symbol_table_thread.h \
	code_gen.h code_gen/code_gen_util.h code_gen/code_gen_expr.h code_gen/code_gen_expr_core.h code_gen/code_gen_expr_binary.h code_gen/code_gen_expr_lambda.h \
	code_gen/code_gen_expr_call.h code_gen/code_gen_expr_thread.h code_gen/code_gen_expr_array.h code_gen/code_gen_expr_static.h \
	code_gen/code_gen_expr_string.h code_gen/code_gen_stmt.h code_gen/code_gen_stmt_loop.h code_gen/code_gen_stmt_capture.h \
	optimizer.h optimizer/optimizer_util.h optimizer/optimizer_tail_call.h optimizer/optimizer_string.h \
	compiler.h debug.h diagnostic.h \
	type_checker.h type_checker/type_checker_util.h type_checker/type_checker_expr.h \
	type_checker/type_checker_expr_call.h type_checker/type_checker_expr_call_core.h \
	type_checker/type_checker_expr_call_array.h type_checker/type_checker_expr_call_string.h \
	type_checker/type_checker_expr_call_file.h type_checker/type_checker_expr_call_time.h \
	type_checker/type_checker_expr_call_net.h type_checker/type_checker_expr_call_random.h \
	type_checker/type_checker_expr_array.h \
	type_checker/type_checker_expr_lambda.h type_checker/type_checker_stmt.h \
	gcc_backend.h \
	runtime/runtime_arena.h runtime/runtime_string.h runtime/runtime_array.h \
	runtime/runtime_file.h runtime/runtime_io.h runtime/runtime_byte.h \
	runtime/runtime_path.h runtime/runtime_date.h runtime/runtime_time.h \
	runtime/runtime_thread.h runtime/runtime_process.h runtime/runtime_net.h \
	runtime/runtime_random_core.h runtime/runtime_random_basic.h runtime/runtime_random_static.h runtime/runtime_random_choice.h runtime/runtime_random_collection.h runtime/runtime_random.h \
	runtime/runtime_uuid.h runtime/runtime_sha1.h runtime/runtime_env.h
BIN_DIR = ../bin
INCLUDE_DIR = $(BIN_DIR)/include
LIB_DIR = $(BIN_DIR)/lib/$(BACKEND)

OBJS = $(addprefix $(BIN_DIR)/, $(notdir $(SRCS:.c=.o)))
DEPS = $(OBJS:.o=.d)

# Backend-specific compiler binary name
ifeq ($(BACKEND),gcc)
TARGET = $(BIN_DIR)/sn-gcc$(EXE_EXT)
else ifeq ($(BACKEND),clang)
TARGET = $(BIN_DIR)/sn-clang$(EXE_EXT)
else ifeq ($(BACKEND),tinycc)
TARGET = $(BIN_DIR)/sn-tcc$(EXE_EXT)
else
TARGET = $(BIN_DIR)/sn-gcc$(EXE_EXT)
endif

# Runtime source files (compiled per-backend to LIB_DIR)
RUNTIME_SRCS = arena.c debug.c runtime.c \
	runtime/runtime_arena.c runtime/runtime_string.c runtime/runtime_array.c \
	runtime/runtime_text_file.c runtime/runtime_binary_file.c runtime/runtime_io.c \
	runtime/runtime_byte.c runtime/runtime_path.c runtime/runtime_date.c runtime/runtime_time.c \
	runtime/runtime_thread.c runtime/runtime_process.c runtime/runtime_net.c \
	runtime/runtime_random_core.c runtime/runtime_random_basic.c runtime/runtime_random_static.c \
	runtime/runtime_random_choice.c runtime/runtime_random_collection.c runtime/runtime_random.c \
	runtime/runtime_uuid.c runtime/runtime_sha1.c runtime/runtime_env.c
RUNTIME_OBJS = $(addprefix $(LIB_DIR)/, $(notdir $(RUNTIME_SRCS:.c=.o)))

VPATH = $(SRCDIR) ast lexer parser type_checker optimizer code_gen runtime symbol_table

.PHONY: all clean runtime runtime-gcc runtime-clang runtime-tinycc runtime-all create-symlink tests tests-clang tests-tcc

all: create-bin-dir $(TARGET) copy-runtime-header runtime create-symlink

# Create sn symlink/copy pointing to the backend-specific binary
create-symlink:
ifeq ($(PLATFORM),windows)
	@rm -f $(BIN_DIR)/sn$(EXE_EXT)
	@cp -f $(TARGET) $(BIN_DIR)/sn$(EXE_EXT)
	@echo "Created copy: sn$(EXE_EXT) -> $(notdir $(TARGET))"
else
	@rm -f $(BIN_DIR)/sn
	@ln -s $(notdir $(TARGET)) $(BIN_DIR)/sn
	@echo "Created symlink: sn -> $(notdir $(TARGET))"
endif

create-bin-dir:
	@mkdir -p $(BIN_DIR)

# Copy runtime headers to include dir so generated code can include them
copy-runtime-header: create-bin-dir
	@mkdir -p $(INCLUDE_DIR)/runtime
	@cp runtime.h $(INCLUDE_DIR)/runtime.h
	@cp runtime/runtime_arena.h $(INCLUDE_DIR)/runtime/runtime_arena.h
	@cp runtime/runtime_string.h $(INCLUDE_DIR)/runtime/runtime_string.h
	@cp runtime/runtime_array.h $(INCLUDE_DIR)/runtime/runtime_array.h
	@cp runtime/runtime_file.h $(INCLUDE_DIR)/runtime/runtime_file.h
	@cp runtime/runtime_io.h $(INCLUDE_DIR)/runtime/runtime_io.h
	@cp runtime/runtime_byte.h $(INCLUDE_DIR)/runtime/runtime_byte.h
	@cp runtime/runtime_path.h $(INCLUDE_DIR)/runtime/runtime_path.h
	@cp runtime/runtime_date.h $(INCLUDE_DIR)/runtime/runtime_date.h
	@cp runtime/runtime_time.h $(INCLUDE_DIR)/runtime/runtime_time.h
	@cp runtime/runtime_thread.h $(INCLUDE_DIR)/runtime/runtime_thread.h
	@cp runtime/runtime_process.h $(INCLUDE_DIR)/runtime/runtime_process.h
	@cp runtime/runtime_net.h $(INCLUDE_DIR)/runtime/runtime_net.h
	@cp runtime/runtime_random_core.h $(INCLUDE_DIR)/runtime/runtime_random_core.h
	@cp runtime/runtime_random_basic.h $(INCLUDE_DIR)/runtime/runtime_random_basic.h
	@cp runtime/runtime_random_static.h $(INCLUDE_DIR)/runtime/runtime_random_static.h
	@cp runtime/runtime_random_choice.h $(INCLUDE_DIR)/runtime/runtime_random_choice.h
	@cp runtime/runtime_random_collection.h $(INCLUDE_DIR)/runtime/runtime_random_collection.h
	@cp runtime/runtime_random.h $(INCLUDE_DIR)/runtime/runtime_random.h
	@cp runtime/runtime_uuid.h $(INCLUDE_DIR)/runtime/runtime_uuid.h
	@cp runtime/runtime_sha1.h $(INCLUDE_DIR)/runtime/runtime_sha1.h
	@cp runtime/runtime_env.h $(INCLUDE_DIR)/runtime/runtime_env.h

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@chmod +x $@

$(BIN_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

# Tests
TEST_SRCDIR = ../tests/unit
TEST_SRCS = $(TEST_SRCDIR)/_all_.c
TEST_OBJS = $(addprefix $(BIN_DIR)/, $(notdir $(TEST_SRCS:.c=.o)))
TEST_TARGET = $(BIN_DIR)/tests$(EXE_EXT)

# Common test dependencies (compiler objects)
TEST_COMPILER_OBJS = $(BIN_DIR)/arena.o $(BIN_DIR)/debug.o $(BIN_DIR)/diagnostic.o $(BIN_DIR)/ast.o $(BIN_DIR)/ast_type.o $(BIN_DIR)/ast_expr.o $(BIN_DIR)/ast_stmt.o $(BIN_DIR)/ast_print.o $(BIN_DIR)/code_gen.o $(BIN_DIR)/code_gen_util.o $(BIN_DIR)/code_gen_expr.o $(BIN_DIR)/code_gen_expr_core.o $(BIN_DIR)/code_gen_expr_binary.o $(BIN_DIR)/code_gen_expr_lambda.o $(BIN_DIR)/code_gen_expr_call.o $(BIN_DIR)/code_gen_expr_call_array.o $(BIN_DIR)/code_gen_expr_call_file.o $(BIN_DIR)/code_gen_expr_call_string.o $(BIN_DIR)/code_gen_expr_call_time.o $(BIN_DIR)/code_gen_expr_call_random.o $(BIN_DIR)/code_gen_expr_call_uuid.o $(BIN_DIR)/code_gen_expr_thread.o $(BIN_DIR)/code_gen_expr_array.o $(BIN_DIR)/code_gen_expr_static.o $(BIN_DIR)/code_gen_expr_string.o $(BIN_DIR)/code_gen_stmt_core.o $(BIN_DIR)/code_gen_stmt_loop.o $(BIN_DIR)/code_gen_stmt_capture.o $(BIN_DIR)/optimizer.o $(BIN_DIR)/optimizer_util.o $(BIN_DIR)/optimizer_tail_call.o $(BIN_DIR)/optimizer_string.o $(BIN_DIR)/lexer.o $(BIN_DIR)/lexer_util.o $(BIN_DIR)/lexer_scan.o $(BIN_DIR)/parser.o $(BIN_DIR)/parser_util.o $(BIN_DIR)/parser_expr.o $(BIN_DIR)/parser_stmt.o $(BIN_DIR)/parser_stmt_decl.o $(BIN_DIR)/parser_stmt_control.o $(BIN_DIR)/symbol_table.o $(BIN_DIR)/symbol_table_core.o $(BIN_DIR)/symbol_table_namespace.o $(BIN_DIR)/symbol_table_thread.o $(BIN_DIR)/type_checker.o $(BIN_DIR)/type_checker_util.o $(BIN_DIR)/type_checker_expr.o $(BIN_DIR)/type_checker_expr_call.o $(BIN_DIR)/type_checker_expr_call_core.o $(BIN_DIR)/type_checker_expr_call_array.o $(BIN_DIR)/type_checker_expr_call_string.o $(BIN_DIR)/type_checker_expr_call_file.o $(BIN_DIR)/type_checker_expr_call_time.o $(BIN_DIR)/type_checker_expr_call_net.o $(BIN_DIR)/type_checker_expr_call_random.o $(BIN_DIR)/type_checker_expr_array.o $(BIN_DIR)/type_checker_expr_lambda.o $(BIN_DIR)/type_checker_stmt.o $(BIN_DIR)/token.o $(BIN_DIR)/file.o

# Runtime objects for GCC backend
TEST_RUNTIME_GCC = $(BIN_DIR)/runtime.o $(BIN_DIR)/runtime_arena.o $(BIN_DIR)/runtime_string.o $(BIN_DIR)/runtime_array.o $(BIN_DIR)/runtime_text_file.o $(BIN_DIR)/runtime_binary_file.o $(BIN_DIR)/runtime_io.o $(BIN_DIR)/runtime_byte.o $(BIN_DIR)/runtime_path.o $(BIN_DIR)/runtime_date.o $(BIN_DIR)/runtime_time.o $(BIN_DIR)/runtime_thread.o $(BIN_DIR)/runtime_process.o $(BIN_DIR)/runtime_net.o $(BIN_DIR)/runtime_random_core.o $(BIN_DIR)/runtime_random_basic.o $(BIN_DIR)/runtime_random_static.o $(BIN_DIR)/runtime_random_choice.o $(BIN_DIR)/runtime_random_collection.o $(BIN_DIR)/runtime_random.o $(BIN_DIR)/runtime_uuid.o $(BIN_DIR)/runtime_sha1.o $(BIN_DIR)/runtime_env.o

# Runtime objects for Clang backend (excluding arena.o and debug.o which are in TEST_COMPILER_OBJS)
TEST_RUNTIME_CLANG = $(BIN_DIR)/lib/clang/runtime.o $(BIN_DIR)/lib/clang/runtime_arena.o $(BIN_DIR)/lib/clang/runtime_string.o $(BIN_DIR)/lib/clang/runtime_array.o $(BIN_DIR)/lib/clang/runtime_text_file.o $(BIN_DIR)/lib/clang/runtime_binary_file.o $(BIN_DIR)/lib/clang/runtime_io.o $(BIN_DIR)/lib/clang/runtime_byte.o $(BIN_DIR)/lib/clang/runtime_path.o $(BIN_DIR)/lib/clang/runtime_date.o $(BIN_DIR)/lib/clang/runtime_time.o $(BIN_DIR)/lib/clang/runtime_thread.o $(BIN_DIR)/lib/clang/runtime_process.o $(BIN_DIR)/lib/clang/runtime_net.o $(BIN_DIR)/lib/clang/runtime_random_core.o $(BIN_DIR)/lib/clang/runtime_random_basic.o $(BIN_DIR)/lib/clang/runtime_random_static.o $(BIN_DIR)/lib/clang/runtime_random_choice.o $(BIN_DIR)/lib/clang/runtime_random_collection.o $(BIN_DIR)/lib/clang/runtime_random.o $(BIN_DIR)/lib/clang/runtime_uuid.o $(BIN_DIR)/lib/clang/runtime_sha1.o $(BIN_DIR)/lib/clang/runtime_env.o

# Runtime objects for TinyCC backend (excluding arena.o and debug.o which are in TEST_COMPILER_OBJS)
TEST_RUNTIME_TCC = $(BIN_DIR)/lib/tinycc/runtime.o $(BIN_DIR)/lib/tinycc/runtime_arena.o $(BIN_DIR)/lib/tinycc/runtime_string.o $(BIN_DIR)/lib/tinycc/runtime_array.o $(BIN_DIR)/lib/tinycc/runtime_text_file.o $(BIN_DIR)/lib/tinycc/runtime_binary_file.o $(BIN_DIR)/lib/tinycc/runtime_io.o $(BIN_DIR)/lib/tinycc/runtime_byte.o $(BIN_DIR)/lib/tinycc/runtime_path.o $(BIN_DIR)/lib/tinycc/runtime_date.o $(BIN_DIR)/lib/tinycc/runtime_time.o $(BIN_DIR)/lib/tinycc/runtime_thread.o $(BIN_DIR)/lib/tinycc/runtime_process.o $(BIN_DIR)/lib/tinycc/runtime_net.o $(BIN_DIR)/lib/tinycc/runtime_random_core.o $(BIN_DIR)/lib/tinycc/runtime_random_basic.o $(BIN_DIR)/lib/tinycc/runtime_random_static.o $(BIN_DIR)/lib/tinycc/runtime_random_choice.o $(BIN_DIR)/lib/tinycc/runtime_random_collection.o $(BIN_DIR)/lib/tinycc/runtime_random.o $(BIN_DIR)/lib/tinycc/runtime_uuid.o $(BIN_DIR)/lib/tinycc/runtime_sha1.o $(BIN_DIR)/lib/tinycc/runtime_env.o

tests: create-bin-dir $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(TEST_COMPILER_OBJS) $(TEST_RUNTIME_GCC)
	$(CC) -o $@ $^ $(LDFLAGS) -lm

# tests-clang: Unit tests linked with Clang-compiled runtime
tests-clang: create-bin-dir $(BIN_DIR)/tests-clang$(EXE_EXT)

$(BIN_DIR)/tests-clang$(EXE_EXT): $(TEST_OBJS) $(TEST_COMPILER_OBJS) $(TEST_RUNTIME_CLANG)
	$(CC) -o $@ $^ $(PTHREAD_FLAG) -lm $(SOCKET_LIBS)

# tests-tcc: Unit tests linked with TinyCC-compiled runtime
tests-tcc: create-bin-dir $(BIN_DIR)/tests-tcc$(EXE_EXT)

$(BIN_DIR)/tests-tcc$(EXE_EXT): $(TEST_OBJS) $(TEST_COMPILER_OBJS) $(TEST_RUNTIME_TCC)
	$(CC) -o $@ $^ $(PTHREAD_FLAG) -lm $(SOCKET_LIBS)

$(BIN_DIR)/%.o: $(TEST_SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@# Preserve config files during clean
	@mkdir -p /tmp/sn_cfg_backup
	@cp $(BIN_DIR)/*.cfg /tmp/sn_cfg_backup/ 2>/dev/null || true
	rm -rf $(BIN_DIR)
	@mkdir -p $(BIN_DIR)
	@cp /tmp/sn_cfg_backup/*.cfg $(BIN_DIR)/ 2>/dev/null || true
	@rm -rf /tmp/sn_cfg_backup

#------------------------------------------------------------------------------
# Runtime builds for different backends
#------------------------------------------------------------------------------

# Build runtime objects for the selected backend
runtime: create-lib-dir $(RUNTIME_OBJS)
	@# Clean up dependency files - only .o files are needed for distribution
	@find $(LIB_DIR) -name "*.d" -delete 2>/dev/null || true
	@echo "Built runtime for $(BACKEND) backend in $(LIB_DIR)"

create-lib-dir:
	@mkdir -p $(LIB_DIR)

# Compile runtime source files to backend-specific lib directory
# Use BACKEND_CC and BACKEND_CFLAGS for backend-specific compilation
# TinyCC doesn't support -MMD/-MP, so we conditionally include them
ifeq ($(BACKEND),tinycc)
BACKEND_DEPFLAGS =
else
BACKEND_DEPFLAGS = -MMD -MP
endif

$(LIB_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(BACKEND_CC) -Wall -std=c99 -O2 $(BACKEND_CFLAGS) $(BACKEND_DEPFLAGS) -D_GNU_SOURCE -I. -Iruntime -c $< -o $@

$(LIB_DIR)/%.o: runtime/%.c
	@mkdir -p $(@D)
	$(BACKEND_CC) -Wall -std=c99 -O2 $(BACKEND_CFLAGS) $(BACKEND_DEPFLAGS) -D_GNU_SOURCE -I. -Iruntime -c $< -o $@

# Convenience targets for each backend
runtime-gcc:
	@$(MAKE) runtime BACKEND=gcc

runtime-clang:
	@$(MAKE) runtime BACKEND=clang

runtime-tinycc:
	@$(MAKE) runtime BACKEND=tinycc

runtime-all: runtime-gcc runtime-clang runtime-tinycc
	@echo "Built runtime for all backends"
