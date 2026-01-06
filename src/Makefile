CC = gcc

# Release build (default): optimized, no sanitizer
CFLAGS = -Wall -Wextra -std=c99 -O3 -flto -MMD -MP -D_GNU_SOURCE -I. -Iruntime
LDFLAGS = -flto -O3 -lpthread

# Debug build: use 'make DEBUG=1' for sanitizer and debug symbols
ifdef DEBUG
CFLAGS = -Wall -Wextra -std=c99 -g -fsanitize=address -fno-omit-frame-pointer -MMD -MP -D_GNU_SOURCE -I. -Iruntime
LDFLAGS = -fsanitize=address -lpthread
endif

SRCDIR = .
SRCS = arena.c file.c runtime.c token.c \
	lexer.c lexer/lexer_util.c lexer/lexer_scan.c \
	ast.c ast/ast_type.c ast/ast_expr.c ast/ast_stmt.c ast/ast_print.c \
	parser.c parser/parser_util.c parser/parser_expr.c parser/parser_stmt.c \
	symbol_table.c \
	code_gen.c code_gen/code_gen_util.c code_gen/code_gen_expr.c code_gen/code_gen_expr_lambda.c \
	code_gen/code_gen_expr_call_array.c code_gen/code_gen_expr_call_file.c \
	code_gen/code_gen_expr_call_string.c code_gen/code_gen_expr_call_time.c \
	code_gen/code_gen_expr_call_random.c code_gen/code_gen_expr_call_uuid.c \
	code_gen/code_gen_expr_array.c code_gen/code_gen_expr_static.c code_gen/code_gen_expr_string.c \
	code_gen/code_gen_stmt.c \
	optimizer.c optimizer/optimizer_util.c optimizer/optimizer_tail_call.c optimizer/optimizer_string.c \
	compiler.c debug.c diagnostic.c \
	type_checker.c type_checker/type_checker_util.c type_checker/type_checker_expr.c \
	type_checker/type_checker_expr_call.c type_checker/type_checker_expr_call_array.c \
	type_checker/type_checker_expr_call_string.c type_checker/type_checker_expr_array.c \
	type_checker/type_checker_expr_lambda.c type_checker/type_checker_stmt.c \
	gcc_backend.c main.c \
	runtime/runtime_arena.c runtime/runtime_string.c runtime/runtime_array.c \
	runtime/runtime_text_file.c runtime/runtime_binary_file.c runtime/runtime_io.c \
	runtime/runtime_byte.c runtime/runtime_path.c runtime/runtime_date.c runtime/runtime_time.c \
	runtime/runtime_thread.c runtime/runtime_process.c runtime/runtime_net.c \
	runtime/runtime_random.c runtime/runtime_uuid.c runtime/runtime_sha1.c \
	runtime/runtime_env.c
HEADERS = arena.h file.h runtime.h token.h \
	lexer.h lexer/lexer_util.h lexer/lexer_scan.h \
	ast.h ast/ast_type.h ast/ast_expr.h ast/ast_stmt.h ast/ast_print.h \
	parser.h parser/parser_util.h parser/parser_expr.h parser/parser_stmt.h \
	symbol_table.h \
	code_gen.h code_gen/code_gen_util.h code_gen/code_gen_expr.h code_gen/code_gen_expr_lambda.h \
	code_gen/code_gen_expr_call.h code_gen/code_gen_expr_array.h code_gen/code_gen_expr_static.h \
	code_gen/code_gen_expr_string.h code_gen/code_gen_stmt.h \
	optimizer.h optimizer/optimizer_util.h optimizer/optimizer_tail_call.h optimizer/optimizer_string.h \
	compiler.h debug.h diagnostic.h \
	type_checker.h type_checker/type_checker_util.h type_checker/type_checker_expr.h \
	type_checker/type_checker_expr_call.h type_checker/type_checker_expr_call_array.h \
	type_checker/type_checker_expr_call_string.h type_checker/type_checker_expr_array.h \
	type_checker/type_checker_expr_lambda.h type_checker/type_checker_stmt.h \
	gcc_backend.h \
	runtime/runtime_arena.h runtime/runtime_string.h runtime/runtime_array.h \
	runtime/runtime_file.h runtime/runtime_io.h runtime/runtime_byte.h \
	runtime/runtime_path.h runtime/runtime_date.h runtime/runtime_time.h \
	runtime/runtime_thread.h runtime/runtime_process.h runtime/runtime_net.h \
	runtime/runtime_random.h runtime/runtime_uuid.h runtime/runtime_sha1.h \
	runtime/runtime_env.h
BIN_DIR = ../bin
OBJS = $(addprefix $(BIN_DIR)/, $(notdir $(SRCS:.c=.o)))
DEPS = $(OBJS:.o=.d)
TARGET = $(BIN_DIR)/sn

VPATH = $(SRCDIR) ast lexer parser type_checker optimizer code_gen runtime

.PHONY: all clean

all: create-bin-dir $(TARGET) copy-runtime-header

create-bin-dir:
	@mkdir -p $(BIN_DIR)

# Copy runtime.h to bin dir so generated code can include it
copy-runtime-header: create-bin-dir
	@mkdir -p $(BIN_DIR)/runtime
	@cp runtime.h $(BIN_DIR)/runtime.h
	@cp runtime/runtime_arena.h $(BIN_DIR)/runtime/runtime_arena.h
	@cp runtime/runtime_string.h $(BIN_DIR)/runtime/runtime_string.h
	@cp runtime/runtime_array.h $(BIN_DIR)/runtime/runtime_array.h
	@cp runtime/runtime_file.h $(BIN_DIR)/runtime/runtime_file.h
	@cp runtime/runtime_io.h $(BIN_DIR)/runtime/runtime_io.h
	@cp runtime/runtime_byte.h $(BIN_DIR)/runtime/runtime_byte.h
	@cp runtime/runtime_path.h $(BIN_DIR)/runtime/runtime_path.h
	@cp runtime/runtime_date.h $(BIN_DIR)/runtime/runtime_date.h
	@cp runtime/runtime_time.h $(BIN_DIR)/runtime/runtime_time.h
	@cp runtime/runtime_thread.h $(BIN_DIR)/runtime/runtime_thread.h
	@cp runtime/runtime_process.h $(BIN_DIR)/runtime/runtime_process.h
	@cp runtime/runtime_net.h $(BIN_DIR)/runtime/runtime_net.h
	@cp runtime/runtime_random.h $(BIN_DIR)/runtime/runtime_random.h
	@cp runtime/runtime_uuid.h $(BIN_DIR)/runtime/runtime_uuid.h
	@cp runtime/runtime_sha1.h $(BIN_DIR)/runtime/runtime_sha1.h
	@cp runtime/runtime_env.h $(BIN_DIR)/runtime/runtime_env.h

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
TEST_TARGET = $(BIN_DIR)/tests

tests: create-bin-dir $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(BIN_DIR)/arena.o $(BIN_DIR)/debug.o $(BIN_DIR)/diagnostic.o $(BIN_DIR)/ast.o $(BIN_DIR)/ast_type.o $(BIN_DIR)/ast_expr.o $(BIN_DIR)/ast_stmt.o $(BIN_DIR)/ast_print.o $(BIN_DIR)/code_gen.o $(BIN_DIR)/code_gen_util.o $(BIN_DIR)/code_gen_expr.o $(BIN_DIR)/code_gen_expr_lambda.o $(BIN_DIR)/code_gen_expr_call_array.o $(BIN_DIR)/code_gen_expr_call_file.o $(BIN_DIR)/code_gen_expr_call_string.o $(BIN_DIR)/code_gen_expr_call_time.o $(BIN_DIR)/code_gen_expr_call_random.o $(BIN_DIR)/code_gen_expr_call_uuid.o $(BIN_DIR)/code_gen_expr_array.o $(BIN_DIR)/code_gen_expr_static.o $(BIN_DIR)/code_gen_expr_string.o $(BIN_DIR)/code_gen_stmt.o $(BIN_DIR)/optimizer.o $(BIN_DIR)/optimizer_util.o $(BIN_DIR)/optimizer_tail_call.o $(BIN_DIR)/optimizer_string.o $(BIN_DIR)/lexer.o $(BIN_DIR)/lexer_util.o $(BIN_DIR)/lexer_scan.o $(BIN_DIR)/parser.o $(BIN_DIR)/parser_util.o $(BIN_DIR)/parser_expr.o $(BIN_DIR)/parser_stmt.o $(BIN_DIR)/symbol_table.o $(BIN_DIR)/type_checker.o $(BIN_DIR)/type_checker_util.o $(BIN_DIR)/type_checker_expr.o $(BIN_DIR)/type_checker_expr_call.o $(BIN_DIR)/type_checker_expr_call_array.o $(BIN_DIR)/type_checker_expr_call_string.o $(BIN_DIR)/type_checker_expr_array.o $(BIN_DIR)/type_checker_expr_lambda.o $(BIN_DIR)/type_checker_stmt.o $(BIN_DIR)/token.o $(BIN_DIR)/file.o $(BIN_DIR)/runtime.o $(BIN_DIR)/runtime_arena.o $(BIN_DIR)/runtime_string.o $(BIN_DIR)/runtime_array.o $(BIN_DIR)/runtime_text_file.o $(BIN_DIR)/runtime_binary_file.o $(BIN_DIR)/runtime_io.o $(BIN_DIR)/runtime_byte.o $(BIN_DIR)/runtime_path.o $(BIN_DIR)/runtime_date.o $(BIN_DIR)/runtime_time.o $(BIN_DIR)/runtime_thread.o $(BIN_DIR)/runtime_process.o $(BIN_DIR)/runtime_net.o $(BIN_DIR)/runtime_random.o $(BIN_DIR)/runtime_uuid.o $(BIN_DIR)/runtime_sha1.o $(BIN_DIR)/runtime_env.o
	$(CC) -o $@ $^ $(LDFLAGS) -lm

$(BIN_DIR)/%.o: $(TEST_SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)
