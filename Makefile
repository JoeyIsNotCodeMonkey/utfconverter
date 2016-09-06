CC = gcc
CFLAGS = -Wall -Werror
DFLAGS = -Wall -Werror -DCSE320 -g
BIN = utfconverter

SRC = $(wildcard *.c)

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean

clean:
	rm -f *.o $(BIN)

.PHONY: debug  

debug: 
	$(CC) $(DFLAGS) $(wildcard *.c) -o $(BIN)