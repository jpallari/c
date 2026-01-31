.SUFFIXES:
CC = cc
LANG_STD = c11

include make/cflags.mk

ifdef ENABLE_RELEASE
	BUILD_DIR = build/release
	include make/cflags.release.mk
else
	BUILD_DIR = build/debug
	include make/cflags.debug.mk
endif

.PHONY: all
all: build test

include make/dirs.mk
include make/ctargets.mk

ifdef ENABLE_MT
	include make/ctargets.mt.mk
endif

CMD_BIN_FILES = $(CMD_NAMES:%=$(BIN_DIR)/%)
TEST_BUILD_FILES = $(TEST_NAMES:%=$(TEST_OBJ_DIR)/%)
TEST_REPORT_FILES = $(TEST_NAMES:%=$(TEST_REPORT_DIR)/%.txt)

.PHONY: build
build: $(CMD_BIN_FILES)

.PHONY: test test-build
test: $(TEST_REPORT_FILES)
test-build: $(TEST_BUILD_FILES)

include make/check.mk
