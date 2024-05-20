OS := $(shell uname)
CFLAGS = -std=c11 -Wall -Wextra

.PHONY: clean debug all executable

all: executable

debug: CFLAGS += -ggdb
debug: executable

release: CFLAGS += -DNDEBUG
release: executable

src/aids.o: src/aids.c src/aids.h
	cc $(CFLAGS) -c -o src/aids.o src/aids.c $(LDFLAGS)

executable: Redic
	./Redic

clean:
	rm -rf Rustris ./src/*.o

Redic: src/main.c src/aids.o
	cc $(CFLAGS) -o Redic src/main.c src/aids.o $(LDFLAGS)

