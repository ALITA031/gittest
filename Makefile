CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS ?=

TARGET := tetris
SRC := src/main.c src/game.c src/render.c src/terminal.c
OBJ := $(SRC:.c=.o)
TEST_TARGET := tests/test_game
TEST_SRC := tests/test_game.c src/game.c

.PHONY: all run clean test debug sanitize

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c src/tetris.h src/terminal.h src/render.h
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) src/tetris.h
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(LDFLAGS)

debug:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -g -O0" all

sanitize:
	$(MAKE) clean
	$(MAKE) CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -g -O1 -fsanitize=address,undefined" LDFLAGS="-fsanitize=address,undefined" test

clean:
	rm -f $(TARGET) $(OBJ) $(TEST_TARGET)
