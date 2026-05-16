CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -Isrc
LDFLAGS = -lm
SRC     = $(wildcard src/*.c)
TARGET  = pico

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $@

debug: CFLAGS += -g -DDEBUG -O0
debug: all

clean:
	rm -f $(TARGET)

.PHONY: all debug clean
