.SUFFIXES:

TEST_FILTERS =

.PHONY: all
all: debug release

.PHONY: debug debug-c11
debug debug-c11:
	make -f Makefile.debug CC=cc LANG_STD=c11 TEST_FILTERS=$(TEST_FILTERS)

.PHONY: debug-gcc-c11
debug-gcc-c11:
	make -f Makefile.debug CC=gcc LANG_STD=c11 TEST_FILTERS=$(TEST_FILTERS)

.PHONY: debug-clang-c11
debug-clang-c11:
	make -f Makefile.debug CC=clang LANG_STD=c11 TEST_FILTERS=$(TEST_FILTERS)

.PHONY: release release-c11
release release-c11:
	make -f Makefile.release CC=cc LANG_STD=c11 TEST_FILTERS=$(TEST_FILTERS)

.PHONY: release-gcc-c11
release-gcc-c11:
	make -f Makefile.release CC=gcc LANG_STD=c11 TEST_FILTERS=$(TEST_FILTERS)

.PHONY: release-clang-c11
release-clang-c11:
	make -f Makefile.release CC=clang LANG_STD=c11 TEST_FILTERS=$(TEST_FILTERS)

.PHONY: lint
lint:
	make -f check.mk lint

.PHONY: format
format:
	make -f check.mk format

.PHONY: clean
clean:
	rm -r build

