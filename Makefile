BIN=balloc_tests
CC=gcc
CFLAGS=-Wall -Werror -Wextra -DB_DEBUG -DB_USE_STDIO -DB_USE_STDDEF -DB_STATISTICS
SRC=balloc.c tests.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

.PHONY: clean
clean:
	rm balloc_tests
