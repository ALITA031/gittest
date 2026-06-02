CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET := tetris
SRC := src/main.c

.PHONY: all run clean debug

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

run: $(TARGET)
	./$(TARGET)

debug:
	$(MAKE) CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -g -O0" all

clean:
	rm -f $(TARGET)
