OS := $(shell uname)
CFLAGS = -std=c11 -Wall -Wextra

.PHONY: clean debug all executable

all: executable

debug: CFLAGS += -ggdb -DDEBUG
debug: executable

release: CFLAGS += -DNDEBUG
release: executable

src/aids.o: src/aids.c src/aids.h
	cc $(CFLAGS) -c -o src/aids.o src/aids.c $(LDFLAGS)

src/resp/resp.o: src/resp/resp.h src/resp/resp.c src/aids.o
	cc $(CFLAGS) -c -o src/resp/resp.o src/resp/resp.c src/aids.o $(LDFLAGS)

src/resp/debug.o: src/resp/debug.h src/resp/resp.h src/aids.o
	cc $(CFLAGS) -c -o src/resp/debug.o src/resp/debug.c src/aids.o $(LDFLAGS)

Redic: src/main.c src/aids.o src/resp/resp.o src/resp/debug.o
	cc $(CFLAGS) -o Redic src/main.c src/aids.o src/resp/resp.o src/resp/debug.o $(LDFLAGS)

executable: Redic
	./Redic

clean:
	rm -rf Rustris ./src/**/*.o
