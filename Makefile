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
DEBUG_OBJ_DIR = $(BUILD_DIR)/debug/obj
DEBUG_CMD_OBJ_DIR = $(BUILD_DIR)/debug/obj/cmd
RELEASE_DIR = $(BUILD_DIR)/release
RELEASE_OBJ_DIR = $(BUILD_DIR)/release/obj
RELEASE_CMD_OBJ_DIR = $(BUILD_DIR)/release/obj/cmd
TEST_BIN_DIR = $(BUILD_DIR)/test

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
	-MMD -MP \
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
ifneq (,$(findstring gcc,$(CC)))
	CFLAGS += -ftree-vectorize
	ifdef VEC_INFO
		CFLAGS += \
			-fopt-info-vec-optimized \
			-fopt-info-vec-missed \
			-fopt-info-vec-all
	endif
endif
ifneq (,$(findstring clang,$(CC)))
	CFLAGS += -fvectorize
	ifdef VEC_INFO
		CFLAGS += \
			-Rpass=loop-vectorize \
			-Rpass-missed=loop-vectorize \
			-Rpass-analysis=loop-vectorize
	endif
endif

# Linker flags
LDFLAGS =
DEBUG_LDFLAGS = $(SAN_FLAGS) -lgcov --coverage
RELEASE_LDFLAGS = -flto

# Local configuration
-include config.mk

# Files
HEADER_FILES = $(wildcard $(INCLUDE_DIR)/*.h)
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
CMD_FILES = $(wildcard $(CMD_DIR)/*.c)
TEST_FILES = $(wildcard $(TEST_DIR)/*.c)
DEBUG_OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(DEBUG_OBJ_DIR)/%.o)
DEBUG_CMD_OBJ_FILES = $(CMD_FILES:$(CMD_DIR)/%.c=$(DEBUG_CMD_OBJ_DIR)/%.o)
CMD_DEBUG_BIN_FILES = $(CMD_FILES:$(CMD_DIR)/%.c=$(DEBUG_DIR)/%)
RELEASE_OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(RELEASE_OBJ_DIR)/%.o)
RELEASE_CMD_OBJ_FILES = $(CMD_FILES:$(CMD_DIR)/%.c=$(RELEASE_CMD_OBJ_DIR)/%.o)
CMD_RELEASE_BIN_FILES = $(CMD_FILES:$(CMD_DIR)/%.c=$(RELEASE_DIR)/%)
TEST_OBJ_FILES = $(TEST_FILES:$(TEST_DIR)/%.c=$(TEST_BIN_DIR)/%.o)
TEST_BIN_FILES = $(TEST_FILES:$(TEST_DIR)/%.c=$(TEST_BIN_DIR)/%)

# Testing
TEST_TARGET_PREFIX = test-
TEST_TARGETS = $(TEST_FILES:$(TEST_DIR)/%.c=$(TEST_TARGET_PREFIX)%)

# Main targets
.PHONY: debug release
debug: $(CMD_DEBUG_BIN_FILES)
release: $(CMD_RELEASE_BIN_FILES)

# Dependencies generated using -MMD -MP
-include $(DEBUG_OBJ_FILES:.o=.d)
-include $(RELEASE_OBJ_FILES:.o=.d)

#
# Build C files to objects
#

$(DEBUG_OBJ_FILES): $(DEBUG_OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

$(RELEASE_OBJ_FILES): $(RELEASE_OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@

$(DEBUG_CMD_OBJ_FILES): $(DEBUG_CMD_OBJ_DIR)/%.o: $(CMD_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $^ -o $@

$(RELEASE_CMD_OBJ_FILES): $(RELEASE_CMD_OBJ_DIR)/%.o: $(CMD_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $^ -o $@

#
# Link C objects to binaries
#

$(CMD_DEBUG_BIN_FILES): $(DEBUG_DIR)/%: $(DEBUG_CMD_OBJ_DIR)/%.o $(DEBUG_OBJ_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@

$(CMD_RELEASE_BIN_FILES): $(RELEASE_DIR)/%: $(RELEASE_CMD_OBJ_DIR)/%.o $(RELEASE_OBJ_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(RELEASE_LDFLAGS) $^ -o $@

#
# Build and run tests
#

.PHONY: lint test test-build format

test: $(TEST_TARGETS)
test-build: $(TEST_BIN_FILES)

$(TEST_OBJ_FILES): $(TEST_BIN_DIR)/%.o: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $^ -o $@

$(TEST_BIN_FILES): $(TEST_BIN_DIR)/%: $(TEST_BIN_DIR)/%.o $(DEBUG_OBJ_FILES)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@

$(TEST_TARGET_PREFIX)%: $(TEST_BIN_DIR)/%
	./$< $(TEST_FILTERS)

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

clean-test:
	rm -rf $(TEST_BIN_DIR)

clean:
	rm -rf $(BUILD_DIR)

