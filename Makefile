.PHONY: all clean

CC = cc
CFLAGS = -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=700 -D_DEFAULT_SOURCE
TARGET = minutka
SOURCES = minutka.c font.c
OBJECTS = $(SOURCES:.c=.o)
DEPS = $(SOURCES:.c=.d)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)
