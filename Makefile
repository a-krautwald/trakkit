CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lasound -lncurses -lm -lpthread

SRC_DIR = src
BIN_DIR = bin
TARGET = $(BIN_DIR)/trakkit

SRCS = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/tui.c \
	$(SRC_DIR)/engine.c \
	$(SRC_DIR)/sample_bank.c \
	$(SRC_DIR)/project_io.c

OBJS = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(SRC_DIR)/*.o $(TARGET)
