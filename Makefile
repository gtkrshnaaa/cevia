CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinclude/
LDFLAGS = -lm

# Directories
SRC_CORE = src/core
SRC_CLI = src/cli
BIN_DIR = bin
LIB_DIR = lib

# Model and data
MODEL_PREFIX ?= data/bin/cevia_id
CORPUS ?= data/corpus_id.txt

# Create directories
$(shell mkdir -p $(BIN_DIR) $(LIB_DIR) data/bin)

# Library source files
LIB_SRCS = $(SRC_CORE)/common.c \
           $(SRC_CORE)/vocab.c \
           $(SRC_CORE)/ngram.c \
           $(SRC_CORE)/pattern.c \
           $(SRC_CORE)/lmModel.c \
           $(SRC_CORE)/cevia_api.c

# CLI source files
CLI_SRCS = $(SRC_CLI)/main.c

# Object files for library
LIB_OBJS = $(LIB_SRCS:.c=.o)

# Embedded model data
VOCAB_OBJ = $(MODEL_PREFIX).vocab.o
UNI_OBJ = $(MODEL_PREFIX).uni.o
BI_OBJ = $(MODEL_PREFIX).bi.o
TRI_OBJ = $(MODEL_PREFIX).tri.o
EMBEDDED_OBJS = $(VOCAB_OBJ) $(UNI_OBJ) $(BI_OBJ) $(TRI_OBJ)

# Targets
STATIC_LIB = $(LIB_DIR)/libcevia.a
SHARED_LIB = $(LIB_DIR)/libcevia.so
CLI_TARGET = $(BIN_DIR)/cevia

.PHONY: all lib cli clean run train eval release help

# Default: build library and CLI
all: lib cli

# Build static library
$(STATIC_LIB): $(LIB_OBJS)
	@echo "Creating static library..."
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^
	@echo "✓ Built: $@"

# Build shared library
$(SHARED_LIB): $(LIB_SRCS)
	@echo "Creating shared library..."
	@mkdir -p $(LIB_DIR)
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LDFLAGS)
	@echo "✓ Built: $@"

# Compile library object files
$(SRC_CORE)/%.o: $(SRC_CORE)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build CLI app (development - uses static library)
$(CLI_TARGET): $(CLI_SRCS) $(STATIC_LIB)
	@echo "Building CLI application..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $(CLI_SRCS) -L$(LIB_DIR) -lcevia $(LDFLAGS)
	@echo "✓ Built: $@"

# Shortcuts
lib: $(STATIC_LIB)
cli: $(CLI_TARGET)

# Release build with embedded model
release: train $(EMBEDDED_OBJS) $(STATIC_LIB)
	@echo "Building release with embedded model..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -DEMBEDDED_MODEL -o $(CLI_TARGET) \
	      $(CLI_SRCS) $(STATIC_LIB) $(EMBEDDED_OBJS) $(LDFLAGS)
	@echo "✓ Release build complete: $(CLI_TARGET)"
	@echo "✓ Single-file executable ready! Size:"
	@ls -lh $(CLI_TARGET) | awk '{print "  " $$5 " - " $$9}'
	@echo "✓ No external files needed!"

# Convert binary files to object files using objcopy
%.o: %
	@echo "Embedding $<..."
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
	         --rename-section .data=.rodata,alloc,load,readonly,data,contents \
	         $< $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(BIN_DIR)/*
	rm -f $(LIB_DIR)/*
	rm -f $(SRC_CORE)/*.o
	rm -f data/bin/*.o
	@echo "✓ Clean complete"

# Run interactive mode
run: $(CLI_TARGET)
	@./$(CLI_TARGET) run --model-prefix $(MODEL_PREFIX)

# Train the model
train: $(CLI_TARGET)
	@echo "Training model with Indonesian corpus..."
	@./$(CLI_TARGET) train $(CORPUS) --model-prefix $(MODEL_PREFIX)
	@echo "✓ Training complete"

# Evaluate model
eval: $(CLI_TARGET)
	@./$(CLI_TARGET) eval $(CORPUS) --model-prefix $(MODEL_PREFIX)

# Help
help:
	@echo "Cevia Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all      - Build library and CLI (default)"
	@echo "  lib      - Build static library only"
	@echo "  cli      - Build CLI application only"
	@echo "  release  - Build single-file executable with embedded model"
	@echo "  train    - Train the model"
	@echo "  eval     - Evaluate the model"
	@echo "  run      - Run interactive mode"
	@echo "  clean    - Remove build artifacts"
	@echo "  help     - Show this help"

