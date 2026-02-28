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

CMD_NAMES += demo reals

$(BIN_DIR)/demo: $(CMD_OBJ_DIR)/demo.o $(OBJ_DIR)/io.o $(OBJ_DIR)/std.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@
$(BIN_DIR)/reals: $(CMD_OBJ_DIR)/reals.o $(OBJ_DIR)/io.o $(OBJ_DIR)/std.o
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ -o $@

#
# Testing
#

.PHONY: test test-build

TEST_NAMES += \
	arena \
	bits \
	bufstream \
	bytes \
	cliargs \
	cstr \
	dynarr \
	math \
	mmap_alloc \
	slice

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

# Bits
$(TEST_OBJ_DIR)/bits.o: test/bits.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/bits: $(TEST_OBJ_DIR)/bits.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/bits.txt: $(TEST_OBJ_DIR)/bits
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

# Math
$(TEST_OBJ_DIR)/math.o: test/math.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/math: $(TEST_OBJ_DIR)/math.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/math.txt: $(TEST_OBJ_DIR)/math
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

# mmap allocator
$(TEST_OBJ_DIR)/mmap_alloc.o: test/mmap_alloc.c include/testr.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/mmap_alloc: $(TEST_OBJ_DIR)/mmap_alloc.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/mmap_alloc.txt: $(TEST_OBJ_DIR)/mmap_alloc
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

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
