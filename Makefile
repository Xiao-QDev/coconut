CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -O2 -Isrc -DPICO_RUNTIME_DIR=\"src\"
LDFLAGS = -lm -lpthread -lws2_32
SRC     = $(wildcard src/*.c src/stdlib/*.c)
TARGET  = pico

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $@

debug: CFLAGS += -g -DDEBUG -O0
debug: all

clean:
	rm -f $(TARGET)

.PHONY: all debug clean
