#include "../include/common.h"

// Hash function for strings (djb2 algorithm)
uint32_t hashFunction(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % HashTableSize;
}

// Create a new hash map
HashMap* createHashMap() {
    HashMap* map = (HashMap*)malloc(sizeof(HashMap));
    if (!map) return NULL;
    
    for (int i = 0; i < HashTableSize; i++) {
        map->table[i] = NULL;
    }
    map->size = 0;
    return map;
}

// Insert or update a key-value pair (separate chaining)
void hashMapPut(HashMap* map, const char* key, uint32_t value) {
    if (!map || !key) return;
    uint32_t index = hashFunction(key);
    KeyValuePair* head = map->table[index];
    // Check if key exists, update
    for (KeyValuePair* p = head; p != NULL; p = p->next) {
        if (strcmp(p->key, key) == 0) {
            p->value = value;
            return;
        }
    }
    // Insert new node at head
    KeyValuePair* node = (KeyValuePair*)malloc(sizeof(KeyValuePair));
    if (!node) return;
    node->key = strdup(key);
    if (!node->key) { free(node); return; }
    node->value = value;
    node->next = head;
    map->table[index] = node;
    map->size++;
}

// Get value by key, returns 0 if not found
uint32_t hashMapGet(HashMap* map, const char* key) {
    if (!map || !key) return 0;
    uint32_t index = hashFunction(key);
    KeyValuePair* p = map->table[index];
    while (p) {
        if (strcmp(p->key, key) == 0) return p->value;
        p = p->next;
    }
    return 0; // Not found
}

// Free hash map memory
void freeHashMap(HashMap* map) {
    if (!map) return;
    
    for (int i = 0; i < HashTableSize; i++) {
        KeyValuePair* p = map->table[i];
        while (p) {
            KeyValuePair* nxt = p->next;
            if (p->key) free(p->key);
            free(p);
            p = nxt;
        }
        map->table[i] = NULL;
    }
    free(map);
}

// Initialize a new sentence
Sentence initSentence(void) {
    Sentence s;
    s.length = 0;
    return s;
}

// Add a token to a sentence
Sentence addToken(Sentence s, const char word[]) {
    if (s.length < MaxTokens) {
        // Ensure the string is null-terminated without triggering truncation warnings
        size_t wordLen = strlen(word);
        size_t copyLen = (wordLen < MaxWordLen) ? wordLen : (MaxWordLen - 1);
        if (copyLen > 0) {
            memcpy(s.sequence[s.length].text, word, copyLen);
        }
        s.sequence[s.length].text[copyLen] = '\0';
        s.length++;
    }
    return s;
}

// Tokenize a line of text into a sentence
void tokenizeLine(const char line[], Sentence* s) {
    if (!line || !s) return;
    
    int i = 0, j = 0;
    char word[MaxWordLen] = {0};
    
    while (line[i] != '\0' && j < MaxWordLen - 1) {
        if (isspace((unsigned char)line[i]) || ispunct((unsigned char)line[i])) {
            if (j > 0) {
                word[j] = '\0';
                *s = addToken(*s, word);
                j = 0;
            }
        } else {
            // Normalize to lowercase for consistent vocabulary
            word[j++] = (char)tolower((unsigned char)line[i]);
        }
        i++;
    }
    
    // Add the last word if exists
    if (j > 0) {
        word[j] = '\0';
        *s = addToken(*s, word);
    }
}
