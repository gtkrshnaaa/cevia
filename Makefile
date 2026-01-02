CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinclude/
LDFLAGS = -lm

# Directories
SRC_DIR = src
BIN_DIR = bin

# Model and data - now using Indonesian corpus
MODEL_PREFIX ?= data/bin/cevia_id
CORPUS ?= data/corpus_id.txt

# Create directories if they don't exist
$(shell mkdir -p $(BIN_DIR) data/bin)

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Target executable
TARGET = $(BIN_DIR)/cevia

.PHONY: all clean run train eval

all: $(TARGET)

# Single-file compilation - compile all sources in one command
$(TARGET): $(SRCS)
	@echo "Compiling Cevia (single-file build)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build complete: $@"

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(BIN_DIR)/*
	rm -f data/bin/*
	@echo "✓ Clean complete"

run: $(TARGET)
	@./$(TARGET) run --model-prefix $(MODEL_PREFIX)

# Train the model using the Indonesian corpus
train: $(TARGET)
	@echo "Training model with Indonesian corpus..."
	@./$(TARGET) train $(CORPUS) --model-prefix $(MODEL_PREFIX)
	@echo "✓ Training complete"

eval: $(TARGET)
	@./$(TARGET) eval $(CORPUS) --model-prefix $(MODEL_PREFIX)

