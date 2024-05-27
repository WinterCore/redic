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

src/resp/resp.o: src/resp/resp.h src/resp/resp.c src/aids.o
	cc $(CFLAGS) -c -o src/resp/resp.o src/resp/resp.c src/aids.o $(LDFLAGS)

Redic: src/main.c src/aids.o src/resp/resp.o
	cc $(CFLAGS) -o Redic src/main.c src/aids.o src/resp/resp.o $(LDFLAGS)


executable: Redic
	./Redic

clean:
	rm -rf Rustris ./src/**/*.o
