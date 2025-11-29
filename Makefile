.PHONY: all clean

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
TARGET = minutka
SOURCES = minutka.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJECTS)
