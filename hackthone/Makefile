CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -O2 -std=c11
LDFLAGS ?= -pthread -lrt

SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build

TARGET = $(BUILD_DIR)/dining_philosophers
TEST_TARGET = $(BUILD_DIR)/test_suite

SRC_FILES = $(SRC_DIR)/simulation.c $(SRC_DIR)/dining_philosophers.c
TEST_FILES = $(SRC_DIR)/simulation.c $(TEST_DIR)/test_suite.c

.PHONY: all clean run test help

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(SRC_FILES) $(SRC_DIR)/simulation.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_FILES) -o $(TARGET) $(LDFLAGS)

$(TEST_TARGET): $(TEST_FILES) $(SRC_DIR)/simulation.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_FILES) -o $(TEST_TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET) -n 5 -m 3 -s 0 -d 400

run-unsafe: $(TARGET)
	./$(TARGET) -n 5 -m 1 -s 1 -d 300

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Available make targets:"
	@echo "  make          - Build dining_philosophers executable in $(BUILD_DIR)"
	@echo "  make run      - Run simulation with 5 philosophers (Deadlock Prevention mode)"
	@echo "  make run-unsafe - Run simulation with Unsafe Circular Wait demo"
	@echo "  make test     - Build and run comprehensive 10-scenario automated test suite"
	@echo "  make clean    - Remove build artifacts"
