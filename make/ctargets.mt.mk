#
# Build C files to objects
#

$(OBJ_DIR)/mt.o: src/mt.c include/mt.h include/std.h
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

#
# Testing
#

TEST_NAMES += ringbuf_spsc

# Ring buffer (SPSC)
$(TEST_OBJ_DIR)/ringbuf_spsc.o: test/ringbuf_spsc.c include/testr.h include/mt.h include/std.h
	@mkdir -p $(TEST_OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
$(TEST_OBJ_DIR)/ringbuf_spsc: $(TEST_OBJ_DIR)/ringbuf_spsc.o $(OBJ_DIR)/testr.o $(OBJ_DIR)/mt.o $(OBJ_DIR)/std.o $(OBJ_DIR)/io.o
	$(CC) $(LDFLAGS) $^ -o $@
$(TEST_REPORT_DIR)/ringbuf_spsc.txt: $(TEST_OBJ_DIR)/ringbuf_spsc
	@mkdir -p $(TEST_REPORT_DIR)
	./$< $(TEST_FILTERS) > $@

