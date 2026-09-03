CC ?= cc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

TARGET := binlens
BUILD_DIR := build
SRC_DIR := src
TEST_DIR := tests
INC_DIR := include

CPPFLAGS ?= -I$(INC_DIR)
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g
COVERAGE_FLAGS ?= --coverage -fprofile-arcs -ftest-coverage
LDFLAGS ?=
LDLIBS ?=

SRCS := $(sort $(wildcard $(SRC_DIR)/*.c))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TEST_SRCS := $(sort $(wildcard $(TEST_DIR)/*.c))
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

FMT_FILES := $(sort $(wildcard $(SRC_DIR)/*.c) $(wildcard $(INC_DIR)/binlens/*.h) $(wildcard $(TEST_DIR)/*.c))

.PHONY: all test clean install uninstall fmt fmt-check coverage coverage-clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(filter-out $(BUILD_DIR)/main.o,$(OBJS))
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(filter-out $(BUILD_DIR)/main.o,$(OBJS)) $(LDFLAGS) -o $@ $(LDLIBS)

test: $(TEST_BINS)
	@set -e; for test_bin in $(TEST_BINS); do $$test_bin; done

COVERAGE_BUILD_DIR := $(BUILD_DIR)/coverage
COVERAGE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(COVERAGE_BUILD_DIR)/%.o,$(SRCS))
COVERAGE_TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(COVERAGE_BUILD_DIR)/%,$(TEST_SRCS))

$(COVERAGE_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(COVERAGE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) -c $< -o $@

$(COVERAGE_BUILD_DIR)/%: $(TEST_DIR)/%.c $(filter-out $(COVERAGE_BUILD_DIR)/main.o,$(COVERAGE_OBJS))
	@mkdir -p $(COVERAGE_BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(COVERAGE_FLAGS) $< $(filter-out $(COVERAGE_BUILD_DIR)/main.o,$(COVERAGE_OBJS)) $(LDFLAGS) -o $@ $(LDLIBS)

coverage: $(COVERAGE_TEST_BINS)
	@set -e; \
	minimum=60; \
	for test_bin in $(COVERAGE_TEST_BINS); do $$test_bin; done; \
	echo "--- Generating coverage report ---"; \
	lcov --capture --directory $(COVERAGE_BUILD_DIR) --output-file $(COVERAGE_BUILD_DIR)/coverage.info --rc lcov_branch_coverage=1 2>/dev/null; \
	lcov --remove $(COVERAGE_BUILD_DIR)/coverage.info '/usr/*' --output-file $(COVERAGE_BUILD_DIR)/coverage.info 2>/dev/null; \
	lcov --summary $(COVERAGE_BUILD_DIR)/coverage.info 2>&1 | head -5; \
	lcov --fail-under-lines=$$minimum --summary $(COVERAGE_BUILD_DIR)/coverage.info 2>/dev/null; \
	echo "Coverage check passed (threshold: $$minimum%)"

coverage-clean:
	rm -rf $(COVERAGE_BUILD_DIR) $(BUILD_DIR)/*.gcda $(BUILD_DIR)/*.gcno

fmt:
	clang-format -i $(FMT_FILES)

fmt-check:
	clang-format --dry-run --Werror $(FMT_FILES)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
