CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -fsanitize=address -fno-omit-frame-pointer -MMD -MP -D_GNU_SOURCE
#CFLAGS = -Wall -Wextra -std=c99 -g -MMD -MP -D_GNU_SOURCE
LDFLAGS = -fsanitize=address
#LDFLAGS = 

SRCDIR = .
SRCS = arena.c file.c runtime.c token.c lexer.c lexer_util.c lexer_scan.c ast.c ast_type.c ast_expr.c ast_stmt.c ast_print.c parser.c parser_util.c parser_expr.c parser_stmt.c symbol_table.c code_gen.c code_gen_util.c code_gen_expr.c code_gen_stmt.c optimizer.c compiler.c debug.c type_checker.c type_checker_util.c type_checker_expr.c type_checker_stmt.c main.c
HEADERS = arena.h file.h runtime.h token.h lexer.h lexer_util.h lexer_scan.h ast.h ast_type.h ast_expr.h ast_stmt.h ast_print.h parser.h parser_util.h parser_expr.h parser_stmt.h symbol_table.h code_gen.h code_gen_util.h code_gen_expr.h code_gen_stmt.h optimizer.h compiler.h debug.h type_checker.h type_checker_util.h type_checker_expr.h type_checker_stmt.h
BIN_DIR = ../bin
OBJS = $(addprefix $(BIN_DIR)/, $(notdir $(SRCS:.c=.o)))
DEPS = $(OBJS:.o=.d)
TARGET = $(BIN_DIR)/sn

VPATH = $(SRCDIR)

.PHONY: all clean

all: create-bin-dir $(TARGET)

create-bin-dir:
	@mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@chmod +x $@

$(BIN_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

# Tests
TEST_SRCDIR = tests
TEST_SRCS = $(TEST_SRCDIR)/_all_.c
TEST_OBJS = $(addprefix $(BIN_DIR)/, $(notdir $(TEST_SRCS:.c=.o)))
TEST_TARGET = $(BIN_DIR)/tests

tests: create-bin-dir $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS) $(BIN_DIR)/arena.o $(BIN_DIR)/debug.o $(BIN_DIR)/ast.o $(BIN_DIR)/ast_type.o $(BIN_DIR)/ast_expr.o $(BIN_DIR)/ast_stmt.o $(BIN_DIR)/ast_print.o $(BIN_DIR)/code_gen.o $(BIN_DIR)/code_gen_util.o $(BIN_DIR)/code_gen_expr.o $(BIN_DIR)/code_gen_stmt.o $(BIN_DIR)/optimizer.o $(BIN_DIR)/lexer.o $(BIN_DIR)/lexer_util.o $(BIN_DIR)/lexer_scan.o $(BIN_DIR)/parser.o $(BIN_DIR)/parser_util.o $(BIN_DIR)/parser_expr.o $(BIN_DIR)/parser_stmt.o $(BIN_DIR)/symbol_table.o $(BIN_DIR)/type_checker.o $(BIN_DIR)/type_checker_util.o $(BIN_DIR)/type_checker_expr.o $(BIN_DIR)/type_checker_stmt.o $(BIN_DIR)/token.o $(BIN_DIR)/file.o $(BIN_DIR)/runtime.o
	$(CC) $(LDFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(TEST_SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BIN_DIR)
