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

CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -g
CPPFLAGS := -Iinclude -Itesting/utils

BUILD_DIR := build
BIN_DIR   := bin

# Library sources
LIB_SRCS := src/IO/Bin_IO.c

# Test sources (unit tests + test helper utilities)
TEST_SRCS := testing/IO/bin_IO_test.c \
             testing/utils/assertion.c

SRCS := $(LIB_SRCS) $(TEST_SRCS)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

TARGET := $(BIN_DIR)/bin_IO_test

.PHONY: all test run clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

test run: $(TARGET)
	./$(TARGET)

$(BUILD_DIR) $(BIN_DIR):
	@mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
