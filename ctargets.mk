#
# Files and directories
#

BUILD_DIR ?= build
OBJ_DIR = $(BUILD_DIR)/obj
CMD_OBJ_DIR = $(OBJ_DIR)/cmd
TEST_OBJ_DIR = $(BUILD_DIR)/test
TEST_REPORT_DIR = $(TEST_OBJ_DIR)/report

TEST_FILES = $(wildcard test/*.c)
TEST_BUILD_FILES = $(TEST_FILES:test/%.c=$(TEST_OBJ_DIR)/%)
TEST_REPORT_FILES = $(TEST_FILES:test/%.c=$(TEST_REPORT_DIR)/%.txt)

#
# Default target
#

.PHONY: build
all: build test

#
# Build C files to objects
#

$(OBJ_DIR)/cliargs.o: src/cliargs.c include/std.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/io.o: src/io.c include/io.h include/std.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/std.o: src/std.c include/std.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(OBJ_DIR)/testr.o: src/testr.c include/io.h include/std.h include/testr.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(CMD_OBJ_DIR)/demo.o: cmd/demo.c include/io.h include/std.h
	@mkdir -p $(CMD_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(CMD_OBJ_DIR)/reals.o: cmd/reals.c include/io.h include/std.h
	@mkdir -p $(CMD_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

#
# Link C objects to binaries
#

.PHONY: build
build: $(BUILD_DIR)/demo $(BUILD_DIR)/reals

$(BUILD_DIR)/demo: $(CMD_OBJ_DIR)/demo.o $(OBJ_DIR)/io.o $(OBJ_DIR)/std.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) $^ -o $@
$(BUILD_DIR)/reals: $(CMD_OBJ_DIR)/reals.o $(OBJ_DIR)/io.o $(OBJ_DIR)/std.o
	@mkdir -p $(BUILD_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

#
# Testing
#

.PHONY: test test-build

test-build: $(TEST_BUILD_FILES)
test: $(TEST_REPORT_FILES)

# Arena
$(TEST_OBJ_DIR)/arena.o: test/arena.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/arena: $(TEST_OBJ_DIR)/arena.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/arena.txt: $(TEST_OBJ_DIR)/arena
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

# Buffered stream
$(TEST_OBJ_DIR)/bufstream.o: test/bufstream.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/bufstream: $(TEST_OBJ_DIR)/bufstream.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/bufstream.txt: $(TEST_OBJ_DIR)/bufstream
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

# Bytes
$(TEST_OBJ_DIR)/bytes.o: test/bytes.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/bytes: $(TEST_OBJ_DIR)/bytes.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/bytes.txt: $(TEST_OBJ_DIR)/bytes
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

# CLI args
$(TEST_OBJ_DIR)/cliargs.o: test/cliargs.c include/testr.h include/std.h include/cliargs.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/cliargs: $(TEST_OBJ_DIR)/cliargs.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o $(OBJ_DIR)/cliargs.o 
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/cliargs.txt: $(TEST_OBJ_DIR)/cliargs
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

# C strings
$(TEST_OBJ_DIR)/cstr.o: test/cstr.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/cstr: $(TEST_OBJ_DIR)/cstr.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/cstr.txt: $(TEST_OBJ_DIR)/cstr
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

# Dynamic array
$(TEST_OBJ_DIR)/dynarr.o: test/dynarr.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/dynarr: $(TEST_OBJ_DIR)/dynarr.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/dynarr.txt: $(TEST_OBJ_DIR)/dynarr
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

# Slice
$(TEST_OBJ_DIR)/slice.o: test/slice.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/slice: $(TEST_OBJ_DIR)/slice.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/slice.txt: $(TEST_OBJ_DIR)/slice
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

#
# Clean-up
#

clean:
	rm -rf $(BUILD_DIR)

