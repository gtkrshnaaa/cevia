#ifndef VocabHeader
#define VocabHeader

#include "common.h"

#define MaxVocabSize 65536  // Maximum number of unique tokens

// Vocabulary structure
typedef struct {
    HashMap* tokenToId;  // token string -> token ID
    char** idToToken;    // array of token strings
    uint32_t size;       // current number of tokens
    uint32_t capacity;   // maximum capacity
} Vocabulary;

// Function declarations
Vocabulary* createVocabulary();
void freeVocabulary(Vocabulary* vocab);
uint32_t getOrAddToken(Vocabulary* vocab, const char* token);
const char* getTokenById(const Vocabulary* vocab, uint32_t id);
void buildVocabularyFromFile(Vocabulary* vocab, const char* filename);
void saveVocabulary(const Vocabulary* vocab, const char* filename);
void loadVocabulary(Vocabulary* vocab, const char* filename);

#endif // VocabHeader
