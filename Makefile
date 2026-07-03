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
LDFLAGS ?=
LDLIBS ?=

SRCS := $(sort $(wildcard $(SRC_DIR)/*.c))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
TEST_SRCS := $(sort $(wildcard $(TEST_DIR)/*.c))
TEST_BINS := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/%,$(TEST_SRCS))

.PHONY: all test clean install uninstall

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
	@for test_bin in $(TEST_BINS); do $$test_bin; done

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
