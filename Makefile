.SUFFIXES:

# Default settings
CC = cc
LANG_STD = c11

# Default target
.PHONY: all
all: build test

# Build flags
include make/cflags.mk
ifdef ENABLE_RELEASE
	BUILD_DIR = build/release
	include make/cflags.release.mk
else
	BUILD_DIR = build/debug
	include make/cflags.debug.mk
endif

# Directories
BUILD_DIR ?= build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj
CMD_OBJ_DIR = $(OBJ_DIR)/cmd
TEST_OBJ_DIR = $(BUILD_DIR)/test
TEST_REPORT_DIR = $(TEST_OBJ_DIR)/report

# C targets
include make/ctargets.mk
ifdef DISABLE_MT
else
	include make/ctargets.mt.mk
endif

# Files for build and test targets
CMD_BIN_FILES = $(CMD_NAMES:%=$(BIN_DIR)/%)
TEST_BUILD_FILES = $(TEST_NAMES:%=$(TEST_OBJ_DIR)/%)
TEST_REPORT_FILES = $(TEST_NAMES:%=$(TEST_REPORT_DIR)/%.txt)

# Build and test targets
.PHONY: build test test-build
build: $(CMD_BIN_FILES)
test: $(TEST_REPORT_FILES)
test-build: $(TEST_BUILD_FILES)

# Static check tools
include make/check.mk

# Generate other makefiles
Makefile.all: Makefile.all.gen.py
	./$< > $@

# Build all combinations
.PHONY: allall allclean
allall: Makefile.all
	$(MAKE) -f $< JOBS=$(JOBS)
allclean: Makefile.all
	$(MAKE) -f $< clean

