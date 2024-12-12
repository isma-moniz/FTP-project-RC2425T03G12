CC = gcc
CFLags = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

.PHONY: downloader
downloader: $(SRC)/main.c
	$(CC) $(CFLAGS) -o download $^

.PHONY: clean
clean:
	rm -rf download