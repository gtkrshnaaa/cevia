#ifndef CommonHeader
#define CommonHeader

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Constants
#define MaxTokens 128
#define MaxWordLen 32
#define MaxN 5  // Maximum n-gram size
#define HashTableSize 1024
// Application metadata
#define AppVersion "0.1.0"
#define DefaultModelPrefix "data/bin/ceviamodel"

// Token structure
typedef struct {
    char text[MaxWordLen];
} Token;

// Sentence structure
typedef struct {
    Token sequence[MaxTokens];
    int length;
} Sentence;

// Key-Value pair for hash map
typedef struct KeyValuePair {
    char* key;
    uint32_t value;
    struct KeyValuePair* next; // linked list for collision resolution
} KeyValuePair;

// HashMap structure
typedef struct {
    KeyValuePair* table[HashTableSize];
    int size;
} HashMap;

// Function declarations
uint32_t hashFunction(const char* str);
HashMap* createHashMap();
void hashMapPut(HashMap* map, const char* key, uint32_t value);
uint32_t hashMapGet(HashMap* map, const char* key);
void freeHashMap(HashMap* map);

// Sentence functions
Sentence initSentence(void);
Sentence addToken(Sentence s, const char word[]);
void tokenizeLine(const char line[], Sentence* s);

#endif // CommonHeader
