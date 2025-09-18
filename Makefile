CC = gcc
LANG_STD = c11
SAN_FLAGS = -fsanitize=address,leak,undefined

# C flags
CFLAGS = \
		-Wall \
		-Wfatal-errors \
		-std=$(LANG_STD) \
		-MMD -MP \
		-fno-omit-frame-pointer \
		-fno-math-errno \
		-ffinite-math-only
CFLAGS += $(EXTRA_CFLAGS)
DEBUG_CFLAGS = -g $(SAN_FLAGS) -DJP_DEBUG
RELEASE_CFLAGS = -O2 -flto

# Linker flags
LDFLAGS =
DEBUG_LDFLAGS = $(SAN_FLAGS)
RELEASE_LDFLAGS = -flto

# Files
HEADER_FILES = $(wildcard src/*.h)
SRC_FILES = $(wildcard src/*.c)
DEBUG_OBJ_FILES = $(SRC_FILES:src/%.c=build/debug/%.o)
RELEASE_OBJ_FILES = $(SRC_FILES:src/%.c=build/release/%.o)

# Local configuration
-include config.mk

debug: build/debug/main
release: build/release/main

# Dependencies generated using -MMD -MP
-include $(DEBUG_OBJ_FILES:.o=.d)
-include $(RELEASE_OBJ_FILES:.o=.d)

build/debug/main: $(DEBUG_OBJ_FILES)
	$(CC) $(LDFLAGS) $(DEBUG_LDFLAGS) $^ -o $@

build/release/main: $(RELEASE_OBJ_FILES)
	$(CC) $(LDFLAGS) $(RELEASE_LDFLAGS) $^ -o $@ 

$(DEBUG_OBJ_FILES): build/debug/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -c $< -o $@

$(RELEASE_OBJ_FILES): build/release/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -c $< -o $@

run-debug: build/debug/main
	$<

run-release: build/release/main
	$<

clean-debug:
	rm -rf build/debug/

clean-release:
	rm -rf build/release/

clean:
	rm -rf build

lint: $(SRC_FILES) $(HEADER_FILES)
	cppcheck -DJP_USE_ASSERT_H --check-level=exhaustive $^

.PHONY: debug release
.PHONY: run-debug run-release
.PHONY: clean-debug clean-release clean
.PHONY: lint
