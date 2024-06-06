OS := $(shell uname)
CFLAGS = -std=c11 -Wall -Wextra

.PHONY: clean debug all executable

all: executable

debug: CFLAGS += -ggdb -DDEBUG
debug: executable

release: CFLAGS += -DNDEBUG
release: executable

SRC=$(wildcard *.c)

Redic: $(SRC)
	cc $(CFLAGS) -c -o $@ $^  $(LDFLAGS)

executable: Redic
	./Redic

clean:
	rm -rf Rustris ./src/**/*.o
