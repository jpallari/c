.POSIX:
.SUFFIXES:
CC = gcc
LANG_STD = c11
SAN_FLAGS = -fsanitize=address,leak,undefined
TEST_FILTERS =

# Directories
INCLUDE_DIR = include
BUILD_DIR = build
SRC_DIR = src
CMD_DIR = cmd
TEST_DIR = test
DEBUG_DIR = $(BUILD_DIR)/debug
DEBUG_OBJ_DIR = $(DEBUG_DIR)/obj
DEBUG_CMD_OBJ_DIR = $(DEBUG_OBJ_DIR)/cmd
DEBUG_TEST_DIR = $(DEBUG_DIR)/test
DEBUG_TEST_REPORT_DIR = $(DEBUG_TEST_DIR)/report
RELEASE_DIR = $(BUILD_DIR)/release
RELEASE_OBJ_DIR = $(RELEASE_DIR)/obj
RELEASE_CMD_OBJ_DIR = $(RELEASE_OBJ_DIR)/cmd
RELEASE_TEST_DIR = $(RELEASE_DIR)/test
RELEASE_TEST_REPORT_DIR = $(RELEASE_TEST_DIR)/report

# Files
HEADER_FILES = \
	$(INCLUDE_DIR)/cliargs.h \
	$(INCLUDE_DIR)/demo.h \
	$(INCLUDE_DIR)/io.h \
	$(INCLUDE_DIR)/std.h \
	$(INCLUDE_DIR)/testr.h
SRC_FILES = \
	$(SRC_DIR)/cliargs.c \
	$(SRC_DIR)/io.c \
	$(SRC_DIR)/std.c \
	$(SRC_DIR)/testr.c
CMD_FILES = \
	$(CMD_DIR)/demo.c \
	$(CMD_DIR)/ping.c \
	$(CMD_DIR)/reals.c
TEST_FILES = \
	$(TEST_DIR)/arena.c \
	$(TEST_DIR)/bytes.c \
	$(TEST_DIR)/cliargs.c \
	$(TEST_DIR)/cstr.c \
	$(TEST_DIR)/dynarr.c \
	$(TEST_DIR)/ringbuf.c \
	$(TEST_DIR)/slice.c

# C flags
CFLAGS = \
	-Wall \
	-Wcast-align \
	-Wcast-qual \
	-Wconversion \
	-Wextra \
	-Wfatal-errors \
	-Wfloat-equal \
	-Wpointer-arith \
	-Wstrict-prototypes \
	-Wswitch-default \
	-Wswitch-enum \
	-Wundef \
	-Wunreachable-code \
	-Wwrite-strings \
	-std=$(LANG_STD) \
	-I$(INCLUDE_DIR) \
	-fno-omit-frame-pointer \
	-fno-math-errno \
	-ffinite-math-only
CFLAGS += $(EXTRA_CFLAGS)
DEBUG_CFLAGS = \
	-g \
	$(SAN_FLAGS) \
	-DJP_DEBUG \
	-fprofile-arcs \
	-ftest-coverage
RELEASE_CFLAGS = -O2 -flto

# To debug vectorization, add these CFLAGS.
# GCC:
# 	CFLAGS += -fopt-info-vec-optimized -fopt-info-vec-missed -fopt-info-vec-all
# Clang:
# 	CFLAGS += -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize

# Linker flags
LDFLAGS =
DEBUG_LDFLAGS = $(SAN_FLAGS) -lgcov --coverage
RELEASE_LDFLAGS = -flto

# Main targets
.PHONY: all
all: debug release test

#
# Build C files to objects
#

$(DEBUG_OBJ_DIR)/cliargs.o: $(SRC_DIR)/cliargs.c $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_OBJ_DIR)/io.o: $(SRC_DIR)/io.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_OBJ_DIR)/std.o: $(SRC_DIR)/std.c $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_OBJ_DIR)/testr.o: $(SRC_DIR)/testr.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h $(INCLUDE_DIR)/testr.h
	@mkdir -p $(DEBUG_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_CMD_OBJ_DIR)/demo.o: $(CMD_DIR)/demo.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_CMD_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_CMD_OBJ_DIR)/reals.o: $(CMD_DIR)/reals.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_CMD_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_CMD_OBJ_DIR)/ping.o: $(CMD_DIR)/ping.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_CMD_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

$(RELEASE_OBJ_DIR)/cliargs.o: $(SRC_DIR)/cliargs.c $(INCLUDE_DIR)/std.h
	@mkdir -p $(RELEASE_OBJ_DIR)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@
$(RELEASE_OBJ_DIR)/io.o: $(SRC_DIR)/io.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(RELEASE_OBJ_DIR)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@
$(RELEASE_OBJ_DIR)/std.o: $(SRC_DIR)/std.c $(INCLUDE_DIR)/std.h
	@mkdir -p $(RELEASE_OBJ_DIR)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@
$(RELEASE_OBJ_DIR)/testr.o: $(SRC_DIR)/std.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h $(INCLUDE_DIR)/testr.h
	@mkdir -p $(RELEASE_OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(RELEASE_CMD_OBJ_DIR)/demo.o: $(CMD_DIR)/demo.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(RELEASE_CMD_OBJ_DIR)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@
$(RELEASE_CMD_OBJ_DIR)/reals.o: $(CMD_DIR)/reals.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(RELEASE_CMD_OBJ_DIR)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@
$(RELEASE_CMD_OBJ_DIR)/ping.o: $(CMD_DIR)/ping.c $(INCLUDE_DIR)/io.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(RELEASE_CMD_OBJ_DIR)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@

#
# Link C objects to binaries
#

.PHONY: debug release

debug: $(DEBUG_DIR)/demo $(DEBUG_DIR)/reals $(DEBUG_DIR)/ping
$(DEBUG_DIR)/demo: $(DEBUG_CMD_OBJ_DIR)/demo.o $(DEBUG_OBJ_DIR)/io.o $(DEBUG_OBJ_DIR)/std.o
	@mkdir -p $(DEBUG_DIR)
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_DIR)/reals: $(DEBUG_CMD_OBJ_DIR)/reals.o $(DEBUG_OBJ_DIR)/io.o $(DEBUG_OBJ_DIR)/std.o
	@mkdir -p $(DEBUG_DIR)
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_DIR)/ping: $(DEBUG_CMD_OBJ_DIR)/ping.o $(DEBUG_OBJ_DIR)/io.o $(DEBUG_OBJ_DIR)/std.o
	@mkdir -p $(DEBUG_DIR)
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@

release: $(RELEASE_DIR)/demo $(RELEASE_DIR)/reals $(RELEASE_DIR)/ping
$(RELEASE_DIR)/demo: $(RELEASE_CMD_OBJ_DIR)/demo.o $(RELEASE_OBJ_DIR)/io.o $(RELEASE_OBJ_DIR)/std.o
	@mkdir -p $(RELEASE_DIR)
	$(CC) $(LDFLAGS) $(RELEASE_LDFLAGS) $^ -o $@
$(RELEASE_DIR)/reals: $(RELEASE_CMD_OBJ_DIR)/reals.o $(RELEASE_OBJ_DIR)/io.o $(RELEASE_OBJ_DIR)/std.o
	@mkdir -p $(RELEASE_DIR)
	$(CC) $(LDFLAGS) $(RELEASE_LDFLAGS) $^ -o $@
$(RELEASE_DIR)/ping: $(RELEASE_CMD_OBJ_DIR)/ping.o $(RELEASE_OBJ_DIR)/io.o $(RELEASE_OBJ_DIR)/std.o
	@mkdir -p $(RELEASE_DIR)
	$(CC) $(LDFLAGS) $(RELEASE_LDFLAGS) $^ -o $@

#
# Build and run tests
#

.PHONY: lint test test-build format

test: \
	$(DEBUG_TEST_REPORT_DIR)/arena.txt \
	$(DEBUG_TEST_REPORT_DIR)/bytes.txt \
	$(DEBUG_TEST_REPORT_DIR)/cliargs.txt \
	$(DEBUG_TEST_REPORT_DIR)/cstr.txt \
	$(DEBUG_TEST_REPORT_DIR)/dynarr.txt \
	$(DEBUG_TEST_REPORT_DIR)/slice.txt

$(DEBUG_TEST_DIR)/arena.o: $(TEST_DIR)/arena.c $(INCLUDE_DIR)/testr.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_TEST_DIR)/arena: $(DEBUG_TEST_DIR)/arena.o $(DEBUG_OBJ_DIR)/testr.o $(DEBUG_OBJ_DIR)/std.o $(DEBUG_OBJ_DIR)/io.o
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_TEST_REPORT_DIR)/arena.txt: $(DEBUG_TEST_DIR)/arena
	@mkdir -p $(DEBUG_TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

$(DEBUG_TEST_DIR)/bytes.o: $(TEST_DIR)/bytes.c $(INCLUDE_DIR)/testr.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_TEST_DIR)/bytes: $(DEBUG_TEST_DIR)/bytes.o $(DEBUG_OBJ_DIR)/testr.o $(DEBUG_OBJ_DIR)/std.o $(DEBUG_OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_TEST_REPORT_DIR)/bytes.txt: $(DEBUG_TEST_DIR)/bytes
	@mkdir -p $(DEBUG_TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

$(DEBUG_TEST_DIR)/cliargs.o: $(TEST_DIR)/cliargs.c $(INCLUDE_DIR)/testr.h $(INCLUDE_DIR)/std.h $(INCLUDE_DIR)/cliargs.h
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_TEST_DIR)/cliargs: $(DEBUG_TEST_DIR)/cliargs.o $(DEBUG_OBJ_DIR)/testr.o $(DEBUG_OBJ_DIR)/std.o $(DEBUG_OBJ_DIR)/io.o $(DEBUG_OBJ_DIR)/cliargs.o 
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_TEST_REPORT_DIR)/cliargs.txt: $(DEBUG_TEST_DIR)/cliargs
	@mkdir -p $(DEBUG_TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

$(DEBUG_TEST_DIR)/cstr.o: $(TEST_DIR)/cstr.c $(INCLUDE_DIR)/testr.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_TEST_DIR)/cstr: $(DEBUG_TEST_DIR)/cstr.o $(DEBUG_OBJ_DIR)/testr.o $(DEBUG_OBJ_DIR)/std.o $(DEBUG_OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_TEST_REPORT_DIR)/cstr.txt: $(DEBUG_TEST_DIR)/cstr
	@mkdir -p $(DEBUG_TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

$(DEBUG_TEST_DIR)/dynarr.o: $(TEST_DIR)/dynarr.c $(INCLUDE_DIR)/testr.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_TEST_DIR)/dynarr: $(DEBUG_TEST_DIR)/dynarr.o $(DEBUG_OBJ_DIR)/testr.o $(DEBUG_OBJ_DIR)/std.o $(DEBUG_OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_TEST_REPORT_DIR)/dynarr.txt: $(DEBUG_TEST_DIR)/dynarr
	@mkdir -p $(DEBUG_TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

$(DEBUG_TEST_DIR)/slice.o: $(TEST_DIR)/slice.c $(INCLUDE_DIR)/testr.h $(INCLUDE_DIR)/std.h
	@mkdir -p $(DEBUG_TEST_DIR)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@
$(DEBUG_TEST_DIR)/slice: $(DEBUG_TEST_DIR)/slice.o $(DEBUG_OBJ_DIR)/testr.o $(DEBUG_OBJ_DIR)/std.o $(DEBUG_OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@
$(DEBUG_TEST_REPORT_DIR)/slice.txt: $(DEBUG_TEST_DIR)/slice
	@mkdir -p $(DEBUG_TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

lint: $(SRC_FILES) $(HEADER_FILES) $(CMD_FILES) $(TEST_FILES)
	cppcheck -DJP_USE_ASSERT_H --check-level=exhaustive $^

format: $(SRC_FILES) $(HEADER_FILES)
	clang-format -i $^

#
# Cleaning build artefacts
#

.PHONY: clean-debug clean-release clean-test clean

clean-debug:
	rm -rf $(DEBUG_DIR)

clean-release:
	rm -rf $(RELEASE_DIR)

clean:
	rm -rf $(BUILD_DIR)

