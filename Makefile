.SUFFIXES:
CC = cc
LANG_STD = c11

include cflags.mk

ifdef ENABLE_RELEASE
	BUILD_DIR = build/release
	include cflags.release.mk
else
	BUILD_DIR = build/debug
	include cflags.debug.mk
endif

.PHONY: all
all: build test

include dirs.mk
include build.mk
include ctargets.mk

ifdef ENABLE_MT
	include ctargets.mt.mk
endif

CMD_BIN_FILES = $(CMD_NAMES:%=$(BIN_DIR)/%)
TEST_BUILD_FILES = $(TEST_NAMES:%=$(TEST_OBJ_DIR)/%)
TEST_REPORT_FILES = $(TEST_NAMES:%=$(TEST_REPORT_DIR)/%.txt)

.PHONY: build
build: $(CMD_BIN_FILES)

.PHONY: test test-build
test: $(TEST_REPORT_FILES)
test-build: $(TEST_BUILD_FILES)

include check.mk
