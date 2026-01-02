#include "../include/vocab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Create a new vocabulary
Vocabulary* createVocabulary() {
    Vocabulary* vocab = (Vocabulary*)malloc(sizeof(Vocabulary));
    if (!vocab) return NULL;
    
    vocab->tokenToId = createHashMap();
    if (!vocab->tokenToId) {
        free(vocab);
        return NULL;
    }
    
    vocab->capacity = MaxVocabSize;
    vocab->idToToken = (char**)calloc(vocab->capacity, sizeof(char*));
    if (!vocab->idToToken) {
        freeHashMap(vocab->tokenToId);
        free(vocab);
        return NULL;
    }
    
    vocab->size = 0;
    
    // Add special tokens
    getOrAddToken(vocab, "<unk>");  // Unknown token
    getOrAddToken(vocab, "<s>");    // Start of sentence
    getOrAddToken(vocab, "</s>");   // End of sentence
    
    return vocab;
}

// Free vocabulary memory
void freeVocabulary(Vocabulary* vocab) {
    if (!vocab) return;
    
    if (vocab->tokenToId) {
        freeHashMap(vocab->tokenToId);
    }
    
    if (vocab->idToToken) {
        for (uint32_t i = 0; i < vocab->size; i++) {
            if (vocab->idToToken[i]) {
                free(vocab->idToToken[i]);
            }
        }
        free(vocab->idToToken);
    }
    
    free(vocab);
}

// Get token ID, add to vocabulary if not exists
uint32_t getOrAddToken(Vocabulary* vocab, const char* token) {
    if (!vocab || !token) return 0;  // 0 is <unk> token ID
    
    // Check if token exists
    uint32_t id = hashMapGet(vocab->tokenToId, token);
    if (id != 0) {
        return id;
    }
    
    // Add new token
    if (vocab->size >= vocab->capacity) {
        // Resize idToToken array if needed
        uint32_t newCapacity = vocab->capacity * 2;
        char** newArray = (char**)realloc(vocab->idToToken, newCapacity * sizeof(char*));
        if (!newArray) return 0;  // Return <unk> on failure
        
        vocab->idToToken = newArray;
        vocab->capacity = newCapacity;
    }
    
    // Allocate and store new token
    char* tokenCopy = strdup(token);
    if (!tokenCopy) return 0;  // Return <unk> on failure
    
    uint32_t newId = vocab->size;
    vocab->idToToken[newId] = tokenCopy;
    hashMapPut(vocab->tokenToId, token, newId);
    vocab->size++;
    
    return newId;
}

// Get token string by ID
const char* getTokenById(const Vocabulary* vocab, uint32_t id) {
    if (!vocab || id >= vocab->size) {
        return "<unk>";
    }
    return vocab->idToToken[id];
}

// Build vocabulary from text file
void buildVocabularyFromFile(Vocabulary* vocab, const char* filename) {
    if (!vocab || !filename) return;
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return;
    }
    
    char line[4096];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline character
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Tokenize line and add tokens to vocabulary
        Sentence s = initSentence();
        tokenizeLine(line, &s);
        
        for (int i = 0; i < s.length; i++) {
            getOrAddToken(vocab, s.sequence[i].text);
        }
    }
    
    fclose(file);
}

// Save vocabulary as C source file for embedding
void saveVocabulary(const Vocabulary* vocab, const char* filename) {
    if (!vocab || !filename) return;
    
    // Change extension to .c
    char cFilename[512];
    snprintf(cFilename, sizeof(cFilename), "src/embedded_vocab.c");
    
    FILE* file = fopen(cFilename, "w");
    if (!file) {
        perror("Failed to create embedded vocab file");
        return;
    }
    
    // Write C source header
    fprintf(file, "// Auto-generated embedded vocabulary\n");
    fprintf(file, "#include <stdint.h>\n\n");
    fprintf(file, "const uint32_t EMBEDDED_VOCAB_SIZE = %u;\n\n", vocab->size);
    
    // Write token strings array
    fprintf(file, "const char* EMBEDDED_VOCAB_TOKENS[] = {\n");
    for (uint32_t i = 0; i < vocab->size; i++) {
        // Escape special characters
        fprintf(file, "    \"");
        const char* token = vocab->idToToken[i];
        for (size_t j = 0; token[j] != '\0'; j++) {
            if (token[j] == '\\') fprintf(file, "\\\\");
            else if (token[j] == '"') fprintf(file, "\\\"");
            else if (token[j] == '\n') fprintf(file, "\\n");
            else if (token[j] == '\t') fprintf(file, "\\t");
            else fprintf(file, "%c", token[j]);
        }
        fprintf(file, "\",\n");
    }
    fprintf(file, "};\n");
    
    fclose(file);
    printf("Generated embedded vocabulary: %s\n", cFilename);
}

// Load vocabulary from file
void loadVocabulary(Vocabulary* vocab, const char* filename) {
    if (!vocab || !filename) return;
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file");
        return;
    }
    
    // Clear existing vocabulary
    if (vocab->tokenToId) freeHashMap(vocab->tokenToId);
    if (vocab->idToToken) {
        for (uint32_t i = 0; i < vocab->size; i++) {
            if (vocab->idToToken[i]) free(vocab->idToToken[i]);
        }
        free(vocab->idToToken);
    }
    
    vocab->tokenToId = createHashMap();
    
    // Read number of tokens
    if (fread(&vocab->size, sizeof(uint32_t), 1, file) != 1) {
        fclose(file);
        return;
    }
    vocab->capacity = vocab->size * 2;  // Allocate extra space
    
    // Allocate token array
    vocab->idToToken = (char**)calloc(vocab->capacity, sizeof(char*));
    if (!vocab->idToToken) {
        fclose(file);
        return;
    }
    
    // Read each token
    for (uint32_t i = 0; i < vocab->size; i++) {
        uint16_t len;
        if (fread(&len, sizeof(uint16_t), 1, file) != 1) {
            fclose(file);
            return;
        }
        
        char* token = (char*)malloc(len + 1);
        if (!token) {
            fclose(file);
            return;
        }
        
        if (fread(token, sizeof(char), len, file) != len) {
            free(token);
            fclose(file);
            return;
        }
        token[len] = '\0';
        
        vocab->idToToken[i] = token;
        hashMapPut(vocab->tokenToId, token, i);
    }
    
    fclose(file);
}
