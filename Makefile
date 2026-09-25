CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC_DIR = src
TEST_DIR = testing
OBJ_DIR = obj
BIN_DIR = bin

# Automatically find all project module files in src/
# NOTE: crypto/ is excluded for now (its OpenSSL headers/include-paths
# aren't sorted out yet). archive.c doesn't depend on it, so run_archive,
# run_io and run_bst all still build fine without it.
SRC_SRCS = $(filter-out $(SRC_DIR)/crypto/%,$(shell find $(SRC_DIR) -name '*.c'))
SRC_OBJS = $(SRC_SRCS:src/%.c=$(OBJ_DIR)/src/%.o)

# Default target executes all tests
all: run_archive run_io run_bst

# Universal rule for compiling src/ files
$(OBJ_DIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Universal rule for compiling testing/ files
$(OBJ_DIR)/testing/%.o: testing/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ---------------------------------------------------------
# Executable Linking Rules
# ---------------------------------------------------------
$(BIN_DIR)/archive_test: $(SRC_OBJS) $(OBJ_DIR)/testing/archive/archive_test.o
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@

$(BIN_DIR)/bin_io_test: $(SRC_OBJS) $(OBJ_DIR)/testing/io/bin_IO_test.o
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@

$(BIN_DIR)/bst_test: $(SRC_OBJS) $(OBJ_DIR)/testing/utils/bst/bst_testing.o
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@

# ---------------------------------------------------------
# Test Execution Targets
# ---------------------------------------------------------
run_archive: $(BIN_DIR)/archive_test
	./$<

run_io: $(BIN_DIR)/bin_io_test
	./$<

run_bst: $(BIN_DIR)/bst_test
	./$<

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) *.bin test_*.bin
	find $(TEST_DIR) -name '*.o' -delete

.PHONY: all clean run_archive run_io run_bst