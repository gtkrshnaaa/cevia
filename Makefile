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

# Embedded model data (object files from binary)
VOCAB_OBJ = $(MODEL_PREFIX).vocab.o
UNI_OBJ = $(MODEL_PREFIX).uni.o
BI_OBJ = $(MODEL_PREFIX).bi.o
TRI_OBJ = $(MODEL_PREFIX).tri.o
EMBEDDED_OBJS = $(VOCAB_OBJ) $(UNI_OBJ) $(BI_OBJ) $(TRI_OBJ)

# Target executable
TARGET = $(BIN_DIR)/cevia

.PHONY: all clean run train eval release

all: $(TARGET)

# Development build (no embedding, uses external files)
$(TARGET): $(SRCS)
	@echo "Compiling Cevia (development build)..."
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✓ Build complete: $@"

# Release build with embedded model data
release: train $(EMBEDDED_OBJS)
	@echo "Building Cevia with embedded model data..."
	$(CC) $(CFLAGS) -DEMBEDDED_MODEL -o $(TARGET) $(SRCS) $(EMBEDDED_OBJS) $(LDFLAGS)
	@echo "✓ Release build complete: $(TARGET)"
	@echo "✓ Single-file executable ready! Size:"
	@ls -lh $(TARGET) | awk '{print "  " $$5 " - " $$9}'
	@echo "✓ No external files needed!"

# Convert binary files to object files using objcopy
%.o: %
	@echo "Embedding $<..."
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
	         --rename-section .data=.rodata,alloc,load,readonly,data,contents \
	         $< $@

clean:
	@echo "Cleaning build artifacts..."
	rm -f $(BIN_DIR)/*
	rm -f data/bin/*.o
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

