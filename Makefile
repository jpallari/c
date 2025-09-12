CC = gcc
LANG_STD = c11
CFLAGS = -Wall -Wfatal-errors -g -std=$(LANG_STD) -MMD -MP
LDFLAGS = 
SRC_FILES = $(wildcard src/*.c)
OBJ_FILES = $(SRC_FILES:src/%.c=build/%.o)

all: main

-include $(OBJ_FILES:.o=.d)
-include $(LIBS_OBJ_FILES:.o=.d)

main: $(OBJ_FILES) $(LIBS_OBJ_FILES)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ_FILES): build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run:
	./main

clean:
	rm -rf main
	rm -rf build

.PHONY: clean run
