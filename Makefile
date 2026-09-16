# Makefile for Cryptos
#
# Builds the library sources under src/ together with the test sources
# under testing/ into a single test runner binary, then (optionally) runs it.
#
# Usage:
#   make            # build the test binary -> bin/bin_IO_test
#   make test       # build (if needed) and run the tests
#   make run        # alias for `make test`
#   make clean      # remove build artifacts

# CC       := gcc
# CFLAGS   := -std=c11 -Wall -Wextra -g
# CPPFLAGS := -Iinclude

# BUILD_DIR := build
# BIN_DIR   := bin

# # Library sources
# LIB_SRCS := src/IO/Bin_IO.c

# # Test sources (unit tests + test helper utilities)
# TEST_SRCS := testing/io/bin_IO_test.c

# SRCS := $(LIB_SRCS) $(TEST_SRCS)
# OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# TARGET := $(BIN_DIR)/bin_IO_test

# .PHONY: all test run clean

# all: $(TARGET)

# $(TARGET): $(OBJS) | $(BIN_DIR)
# 	$(CC) $(OBJS) -o $@

# $(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
# 	@mkdir -p $(dir $@)
# 	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# test run: $(TARGET)
# 	./$(TARGET)

# $(BUILD_DIR) $(BIN_DIR):
# 	@mkdir -p $@

# clean:
# 	rm -rf $(BUILD_DIR) $(BIN_DIR)


####

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude

SRC_DIR = src
TEST_DIR = testing
OBJ_DIR = obj
BIN_DIR = bin

TARGET = $(BIN_DIR)/archive_test

# Automatically find all project module files in src/
SRC_SRCS = $(shell find $(SRC_DIR) -name '*.c')

# Define the specific test file to run
TEST_SRC = $(TEST_DIR)/archive/read_write/archive_test.c

# Map .c files to .o files inside the obj/ directory while retaining their nested paths[cite: 5]
OBJS = $(SRC_SRCS:%.c=$(OBJ_DIR)/%.o) $(TEST_SRC:%.c=$(OBJ_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@

# Universal rule to compile any .c file into obj/ matching its original directory structure
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) *.bin

.PHONY: all run clean