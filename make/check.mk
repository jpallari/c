.PHONY: lint format

HEADER_FILES = $(wildcard include/*.h)
SRC_FILES = $(wildcard src/*.c)
CMD_FILES = $(wildcard cmd/*.c)
TEST_FILES = $(wildcard test/*.c)

lint: $(SRC_FILES) $(HEADER_FILES) $(CMD_FILES) $(TEST_FILES)
	cppcheck -DJP_USE_ASSERT_H --check-level=exhaustive $^

format: $(SRC_FILES) $(HEADER_FILES) $(CMD_FILES) $(TEST_FILES)
	clang-format -i $^

