CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinclude/
LDFLAGS = -lm

# Build directory structure
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin
EMBED_DIR = $(BUILD_DIR)/embed

# Model and data
MODEL_PREFIX ?= data/bin/cevia_id
CORPUS ?= data/corpus_id.txt

# Create all build directories
$(shell mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR) $(EMBED_DIR) data/bin)

# Source directories
SRC_CORE = src/core
SRC_CLI = src/cli

# Library source files
LIB_SRCS = $(SRC_CORE)/common.c \
           $(SRC_CORE)/vocab.c \
           $(SRC_CORE)/ngram.c \
           $(SRC_CORE)/pattern.c \
           $(SRC_CORE)/lmModel.c \
           $(SRC_CORE)/cevia_api.c

# CLI source files
CLI_SRCS = $(SRC_CLI)/main.c

# Object files (in build/obj/)
LIB_OBJS = $(patsubst $(SRC_CORE)/%.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))

# Embedded model data (in build/embed/)
VOCAB_OBJ = $(EMBED_DIR)/cevia_id.vocab.o
UNI_OBJ = $(EMBED_DIR)/cevia_id.uni.o
BI_OBJ = $(EMBED_DIR)/cevia_id.bi.o
TRI_OBJ = $(EMBED_DIR)/cevia_id.tri.o
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
$(OBJ_DIR)/%.o: $(SRC_CORE)/%.c
	@mkdir -p $(OBJ_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

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

# Convert binary model files to object files using objcopy
$(EMBED_DIR)/cevia_id.%.o: data/bin/cevia_id.%
	@echo "Embedding $<..."
	@mkdir -p $(EMBED_DIR)
	@objcopy -I binary -O elf64-x86-64 -B i386:x86-64 \
	         --rename-section .data=.rodata,alloc,load,readonly,data,contents \
	         $< $@

# Clean all build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)
	@echo "✓ Clean complete"

# Clean and rebuild everything
rebuild: clean all

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
	@echo "  clean    - Remove all build artifacts"
	@echo "  rebuild  - Clean and rebuild everything"
	@echo "  help     - Show this help"
	@echo ""
	@echo "Build Structure:"
	@echo "  build/obj/    - Object files (.o)"
	@echo "  build/lib/    - Libraries (.a, .so)"
	@echo "  build/bin/    - Executables"
	@echo "  build/embed/  - Embedded model data (.o)"

