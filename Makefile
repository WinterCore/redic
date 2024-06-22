OS := $(shell uname)
CFLAGS = -std=c11 -Wall -Wextra

.PHONY: clean debug all


debug: CFLAGS += -ggdb -DDEBUG
debug: Redic

release: CFLAGS += -DNDEBUG

SRC_C=$(shell find . -name "*.c")
SRC_ALL=$(shell find . -name "*.c" -o -name '*.h')

Redic: $(SRC)
	cc $(CFLAGS) -o Redic $(SRC_C) $(LDFLAGS)

all: Redic

clean:
	rm -rf Redic ./src/*.o ./src/**/*.o ./Redic.dSYM
