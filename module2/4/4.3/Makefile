CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_GNU_SOURCE
TARGET = phonebook
SRC = main.c contact.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
