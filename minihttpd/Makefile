CC=gcc
CFLAGS=-Wall -Wextra -O2 -std=gnu11 -D_GNU_SOURCE -Iinclude
SRC=src/main.c src/server.c src/http.c src/mime.c src/files.c
TARGET=minihttpd

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
