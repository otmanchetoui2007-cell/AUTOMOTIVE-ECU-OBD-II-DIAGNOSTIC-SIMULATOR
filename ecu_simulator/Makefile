
CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS = -lm

TARGET = output/ecu_simulator

SOURCES = main.c \
          src/can_interface.c \
          src/engine_model.c \
          src/obd2.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p output
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all clean
