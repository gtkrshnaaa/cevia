CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinclude/
LDFLAGS = -lm

# Source files
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Model and data
MODEL_PREFIX ?= data/bin/ceviamodel
CORPUS ?= data/corpus.txt

# Create directories if they don't exist
$(shell mkdir -p $(BUILD_DIR) $(BIN_DIR) data/bin)

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Targets
TARGET = $(BIN_DIR)/cevia

.PHONY: all clean run train eval

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/* $(BIN_DIR)/* data/bin/*

run: $(TARGET)
	@./$(TARGET) run

# Train the model using the corpus during build flow
train: $(TARGET)
	@./$(TARGET) train $(CORPUS)

eval: $(TARGET)
	@./$(TARGET) eval $(CORPUS)
