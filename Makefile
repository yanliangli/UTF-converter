CC = gcc
SRC = src/*.c
INCLUDE = include/
BIN = ./bin/utf
CFLAGS = -g -Wall -Werror -pedantic -Wextra -std=c90
LDFLAGS = -lm
REQ = $(SRC) include/*.h
DES = ./bin
BUILD = ./build/

all: $(BIN)

$(BIN): $(REQ) utf.o
	@mkdir -p bin
	$(CC) $(CFLAGS) -I  $(SRC) $(BUILD)utf.o -o $(BIN) $(LDFLAGS) 

utf.o: ./src/utfconverter.c 
	@mkdir -p build
	gcc $(CFLAGS) -c $^ -o $(BUILD)$@

.PHONY: clean

clean:
	rm -rf $(BIN)

