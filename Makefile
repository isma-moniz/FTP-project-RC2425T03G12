CC = gcc
CFLags = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: download
download: $(SRC)/main.c
	$(CC) $(CFLAGS) -o download $^

.PHONY: clean
clean:
	rm -rf download